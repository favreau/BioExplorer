/*
    Copyright 2015 - 2024 Blue Brain Project / EPFL

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "VolumeLoader.h"

#include <platform/core/common/utils/FileSystem.h>
#include <platform/core/common/utils/StringUtils.h>
#include <platform/core/common/utils/Utils.h>
#include <platform/core/engineapi/Model.h>
#include <platform/core/engineapi/Scene.h>
#include <platform/core/engineapi/SharedDataVolume.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace
{
using Property = core::Property;
const Property PROP_DIMENSIONS = {"dimensions", std::array<int32_t, 3>({{0, 0, 0}}), {"Dimensions"}};
const Property PROP_SPACING = {"spacing", std::array<double, 3>({{1, 1, 1}}), {"Spacing"}};
const Property PROP_TYPE = {"type",
                            core::enumToString(core::DataType::UINT8),
                            core::enumNames<core::DataType>(),
                            {"Type"}};
} // namespace

namespace core
{
namespace
{
template <size_t M, typename T>
std::string to_string(const glm::vec<M, T>& vec)
{
    std::stringstream stream;
    stream << vec;
    return stream.str();
}

template <typename T>
std::array<T, 3> parseArray3(const std::string& str, std::function<T(std::string)> conv)
{
    const auto v = core::string_utils::split(str, ' ');
    if (v.size() != 3)
        throw std::runtime_error("Not exactly 3 values for mhd array");
    return {{conv(v[0]), conv(v[1]), conv(v[2])}};
}

std::map<std::string, std::string> parseMHD(const std::string& filename)
{
    std::ifstream infile(filename);
    if (!infile.good())
        throw std::runtime_error("Could not open file " + filename);

    // Sample MHD File:
    //
    // ObjectType = Image
    // DimSize = 1 2 3
    // ElementSpacing = 0.1 0.2 0.3
    // ElementType = MET_USHORT
    // ElementDataFile = BS39.raw

    std::map<std::string, std::string> result;
    std::string line;
    size_t ctr = 1;
    while (std::getline(infile, line))
    {
        const auto v = string_utils::split(line, '=');
        if (v.size() != 2)
            throw std::runtime_error("Could not parse line " + std::to_string(ctr));
        auto key = v[0];
        auto value = v[1];
        string_utils::trim(key);
        string_utils::trim(value);

        result[key] = value;
        ++ctr;
    }

    return result;
}

DataType dataTypeFromMET(const std::string& type)
{
    if (type == "MET_FLOAT")
        return DataType::FLOAT;
    else if (type == "MET_DOUBLE")
        return DataType::DOUBLE;
    else if (type == "MET_UCHAR")
        return DataType::UINT8;
    else if (type == "MET_USHORT")
        return DataType::UINT16;
    else if (type == "MET_UINT")
        return DataType::UINT32;
    else if (type == "MET_CHAR")
        return DataType::INT8;
    else if (type == "MET_SHORT")
        return DataType::INT16;
    else if (type == "MET_INT")
        return DataType::INT32;
    else
        throw std::runtime_error("Unknown data type " + type);
}

Vector2f dataRangeFromType(DataType type)
{
    switch (type)
    {
    case DataType::UINT8:
        return {std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max()};
    case DataType::UINT16:
        return {std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()};
    case DataType::UINT32:
        return {std::numeric_limits<uint32_t>::min() / 100, std::numeric_limits<uint32_t>::max() / 100};
    case DataType::INT8:
        return {std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max()};
    case DataType::INT16:
        return {std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()};
    case DataType::INT32:
        return {std::numeric_limits<int32_t>::min() / 100, std::numeric_limits<int32_t>::max() / 100};
    case DataType::FLOAT:
    case DataType::DOUBLE:
    default:
        return {0, 1};
    }
}
} // namespace

RawVolumeLoader::RawVolumeLoader(Scene& scene)
    : Loader(scene)
{
}

bool RawVolumeLoader::isSupported(const std::string& storage, const std::string& extension) const
{
    return extension == "raw";
}

ModelDescriptorPtr RawVolumeLoader::importFromBlob(Blob&& blob, const LoaderProgress& callback,
                                                   const PropertyMap& properties) const
{
    return _loadVolume(blob.name, callback, properties,
                       [&blob](auto volume) { volume->mapData(std::move(blob.data)); });
}

ModelDescriptorPtr RawVolumeLoader::importFromStorage(const std::string& storage, const LoaderProgress& callback,
                                                      const PropertyMap& properties) const
{
    return _loadVolume(storage, callback, properties, [storage](auto volume) { volume->mapData(storage); });
}

ModelDescriptorPtr RawVolumeLoader::_loadVolume(const std::string& filename, const LoaderProgress& callback,
                                                const PropertyMap& propertiesTmp,
                                                const std::function<void(SharedDataVolumePtr)>& mapData) const
{
    // Fill property map since the actual property types are known now.
    PropertyMap properties = getProperties();
    properties.merge(propertiesTmp);

    callback.updateProgress("Parsing volume file ...", 0.f);

    const auto dimensions = toGlmVec(properties.getProperty<std::array<int32_t, 3>>(PROP_DIMENSIONS.name));
    const auto spacing = toGlmVec(properties.getProperty<std::array<double, 3>>(PROP_SPACING.name));
    const auto type = stringToEnum<DataType>(properties.getProperty<std::string>(PROP_TYPE.name));

    if (glm::compMul(dimensions) == 0)
        throw std::runtime_error("Volume dimensions are empty");

    auto dataRange = dataRangeFromType(type);
    auto model = _scene.createModel();
    auto volume = model->createSharedDataVolume(dimensions, spacing, type);
    volume->setDataRange(dataRange);

    callback.updateProgress("Loading voxels ...", 0.5f);
    mapData(volume);

    callback.updateProgress("Adding model ...", 1.f);
    model->addVolume(VOLUME_MATERIAL_ID, volume);

    Transformation transformation;
    transformation.setRotationCenter(model->getBounds().getCenter());
    dataRange = volume->getDataRange();
    auto modelDescriptor = std::make_shared<ModelDescriptor>(
        std::move(model), filename,
        ModelMetadata{{"Data type", properties.getProperty<std::string>(PROP_TYPE.name)},
                      {"Dimensions", std::to_string(dimensions.x) + "," + std::to_string(dimensions.y) + "," +
                                         std::to_string(dimensions.z)},
                      {"Element Spacing",
                       std::to_string(spacing.x) + "," + std::to_string(spacing.y) + "," + std::to_string(spacing.z)},
                      {"Data range", std::to_string(dataRange.x) + "," + std::to_string(dataRange.y)}});
    modelDescriptor->setTransformation(transformation);
    return modelDescriptor;
}

std::string RawVolumeLoader::getName() const
{
    return "raw-volume";
}

std::vector<std::string> RawVolumeLoader::getSupportedStorage() const
{
    return {"raw"};
}

PropertyMap RawVolumeLoader::getProperties() const
{
    PropertyMap pm;
    pm.setProperty(PROP_DIMENSIONS);
    pm.setProperty(PROP_SPACING);
    pm.setProperty(PROP_TYPE);
    return pm;
}
////////////////////////////////////////////////////////////////////////////

MHDVolumeLoader::MHDVolumeLoader(Scene& scene)
    : Loader(scene)
{
}

bool MHDVolumeLoader::isSupported(const std::string& storage, const std::string& extension) const
{
    return extension == "mhd";
}

ModelDescriptorPtr MHDVolumeLoader::importFromBlob(Blob&& blob, const LoaderProgress&,
                                                   const PropertyMap& properties) const
{
    throw std::runtime_error("Volume loading from blob is not supported");
}

ModelDescriptorPtr MHDVolumeLoader::importFromStorage(const std::string& storage, const LoaderProgress& callback,
                                                      const PropertyMap&) const
{
    std::string volumeFile = storage;
    const auto mhd = parseMHD(storage);

    // Check all keys present
    for (const auto key : {"ObjectType", "DimSize", "ElementSpacing", "ElementType", "ElementDataFile"})
        if (mhd.find(key) == mhd.end())
            throw std::runtime_error("Missing key " + std::string(key));

    if (mhd.at("ObjectType") != "Image")
        throw std::runtime_error("Wrong object type for mhd file");

    const auto dimensions = parseArray3<int32_t>(mhd.at("DimSize"), [](const auto& s) { return stoi(s); });
    const auto spacing = parseArray3<double>(mhd.at("ElementSpacing"), [](const auto& s) { return stod(s); });
    const auto type = dataTypeFromMET(mhd.at("ElementType"));

    fs::path path = mhd.at("ElementDataFile");
    if (!path.is_absolute())
    {
        auto basePath = fs::path(storage).parent_path();
        path = fs::canonical(basePath / path);
    }
    volumeFile = path.string();

    PropertyMap properties;
    properties.setProperty({PROP_DIMENSIONS.name, dimensions, PROP_DIMENSIONS.metaData});
    properties.setProperty({PROP_SPACING.name, spacing, PROP_SPACING.metaData});
    properties.setProperty({PROP_TYPE.name, core::enumToString(type), PROP_TYPE.enums, PROP_TYPE.metaData});

    return RawVolumeLoader(_scene).importFromStorage(volumeFile, callback, properties);
}

std::string MHDVolumeLoader::getName() const
{
    return "mhd-volume";
}

std::vector<std::string> MHDVolumeLoader::getSupportedStorage() const
{
    return {"mhd"};
}
} // namespace core

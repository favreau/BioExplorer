/*
    Copyright 2020 - 2024 Blue Brain Project / EPFL

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

#include "SonataCacheLoader.h"

#include <plugin/neuroscience/common/Types.h>

#include <plugin/neuroscience/common/MorphologyLoader.h>
#include <plugin/neuroscience/neuron/SpikeSimulationHandler.h>
#include <plugin/neuroscience/neuron/VoltageSimulationHandler.h>

#include <common/Logs.h>

#include <platform/core/common/Properties.h>

#include <platform/core/engineapi/Material.h>
#include <platform/core/engineapi/Model.h>
#include <platform/core/engineapi/Scene.h>

#include <fstream>

using namespace core;

namespace sonataexplorer
{
namespace io
{
namespace loader
{
using namespace core;
using namespace neuroscience;
using namespace neuron;

const size_t CACHE_VERSION_1 = 1;
const size_t CACHE_VERSION_2 = 2;
const size_t CACHE_VERSION_3 = 3;
const size_t CACHE_VERSION_4 = 4;

const std::string LOADER_NAME = "Sonata cache";
const std::string SUPPORTED_EXTENTION_SONATA_CACHE = "soc";

const Property PROP_LOAD_SPHERES = {"spheres", true, {"Load spheres"}};
const Property PROP_LOAD_CYLINDERS = {"cylinders", true, {"Load cylinders"}};
const Property PROP_LOAD_CONES = {"cones", true, {"Load cones"}};
const Property PROP_LOAD_MESHES = {"meshes", true, {"Load meshes"}};
const Property PROP_LOAD_STREAMLINES = {"streamlines", true, {"Load streamlines"}};
const Property PROP_LOAD_SDF = {"sdf", true, {"Load signed distance field geometry"}};
const Property PROP_LOAD_SIMULATION = {"simulation", true, {"Attach simulation data (if applicable"}};

SonataCacheLoader::SonataCacheLoader(Scene& scene, PropertyMap&& loaderParams)
    : Loader(scene)
    , _defaults(loaderParams)
{
}

std::string SonataCacheLoader::getName() const
{
    return LOADER_NAME;
}

std::vector<std::string> SonataCacheLoader::getSupportedStorage() const
{
    return {SUPPORTED_EXTENTION_SONATA_CACHE};
}

bool SonataCacheLoader::isSupported(const std::string& /*filename*/, const std::string& extension) const
{
    const std::set<std::string> types = {SUPPORTED_EXTENTION_SONATA_CACHE};
    return types.find(extension) != types.end();
}

ModelDescriptorPtr SonataCacheLoader::importFromBlob(Blob&& /*blob*/, const LoaderProgress& /*callback*/,
                                                     const PropertyMap& /*properties*/) const
{
    throw std::runtime_error("Loading circuit from blob is not supported");
}

std::string SonataCacheLoader::_readString(std::ifstream& buffer) const
{
    size_t size;
    buffer.read((char*)&size, sizeof(size_t));
    std::vector<char> str;
    str.resize(size + 1, 0);
    buffer.read(&str[0], size);
    return str.data();
}

ModelDescriptorPtr SonataCacheLoader::importFromStorage(const std::string& path, const LoaderProgress& callback,
                                                        const PropertyMap& properties) const
{
    PropertyMap props = _defaults;
    props.merge(properties);

    callback.updateProgress("Loading cache...", 0);
    PLUGIN_INFO("Loading model from cache file: " << path);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.good())
        PLUGIN_THROW("Could not open cache file " + path);

    // File version
    size_t version;
    file.read((char*)&version, sizeof(size_t));

    PLUGIN_INFO("Version: " << version);

    auto model = _scene.createModel();

    // Geometry
    size_t nbSpheres = 0;
    size_t nbCylinders = 0;
    size_t nbCones = 0;
    size_t nbMeshes = 0;
    size_t nbVertices = 0;
    size_t nbIndices = 0;
    size_t nbNormals = 0;
    size_t nbTexCoords = 0;

    // Metadata
    size_t nbElements;
    ModelMetadata metadata;
    file.read((char*)&nbElements, sizeof(size_t));
    for (size_t i = 0; i < nbElements; ++i)
        metadata[_readString(file)] = _readString(file);

    size_t nbMaterials;
    file.read((char*)&nbMaterials, sizeof(size_t));

    // Materials
    size_t materialId;
    for (size_t i = 0; i < nbMaterials; ++i)
    {
        callback.updateProgress("Materials (" + std::to_string(i + 1) + "/" + std::to_string(nbMaterials) + ")",
                                0.1f * float(i) / float(nbMaterials));
        file.read((char*)&materialId, sizeof(size_t));

        auto name = _readString(file);
        auto material = model->createMaterial(materialId, name);

        Vector3f value3f;
        file.read((char*)&value3f, sizeof(Vector3f));
        material->setDiffuseColor(value3f);
        file.read((char*)&value3f, sizeof(Vector3f));
        material->setSpecularColor(value3f);
        float value;
        file.read((char*)&value, sizeof(float));
        material->setSpecularExponent(value);
        file.read((char*)&value, sizeof(float));
        material->setReflectionIndex(value);
        file.read((char*)&value, sizeof(float));
        material->setOpacity(value);
        file.read((char*)&value, sizeof(float));
        material->setRefractionIndex(value);
        file.read((char*)&value, sizeof(float));
        material->setEmission(value);
        file.read((char*)&value, sizeof(float));
        material->setGlossiness(value);

        if (version == CACHE_VERSION_1)
        {
            bool userData;
            file.read((char*)&userData, sizeof(bool));
            material->setCastUserData(userData);

            size_t shadingMode;
            file.read((char*)&shadingMode, sizeof(size_t));
            material->setShadingMode(static_cast<MaterialShadingMode>(shadingMode));
        }

        if (version >= CACHE_VERSION_2)
        {
            int32_t userData;
            file.read((char*)&userData, sizeof(int32_t));
            material->setCastUserData(userData);

            int32_t shadingMode;
            file.read((char*)&shadingMode, sizeof(int32_t));
            material->setShadingMode(static_cast<MaterialShadingMode>(shadingMode));
        }

        if (version == CACHE_VERSION_3)
        {
            bool clipped;
            file.read((char*)&clipped, sizeof(bool));
            material->setClippingMode(clipped ? MaterialClippingMode::plane : MaterialClippingMode::no_clipping);
        }

        if (version >= CACHE_VERSION_4)
        {
            int32_t clippingMode;
            file.read((char*)&clippingMode, sizeof(int32_t));
            material->setClippingMode(static_cast<MaterialClippingMode>(clippingMode));
        }
    }

    uint64_t bufferSize{0};

    // Spheres
    callback.updateProgress("Spheres", 0.2f);
    file.read((char*)&nbSpheres, sizeof(size_t));
    for (size_t i = 0; i < nbSpheres; ++i)
    {
        file.read((char*)&materialId, sizeof(size_t));
        file.read((char*)&nbElements, sizeof(size_t));

        struct SphereV1
        {
            Vector3f center;
            float radius;
            float timestamp;
            float value;
        };

        if (props.getProperty<bool>(PROP_LOAD_SPHERES.name))
        {
            callback.updateProgress("Spheres (" + std::to_string(i + 1) + "/" + std::to_string(nbSpheres) + ")",
                                    0.2f + 0.1f * float(i) / float(nbSpheres));
            auto& spheres = model->getSpheres()[materialId];
            spheres.resize(nbElements);

            if (version >= CACHE_VERSION_2)
            {
                bufferSize = nbElements * sizeof(Sphere);
                file.read((char*)spheres.data(), bufferSize);
            }
            else
            {
                std::vector<SphereV1> spheresV1;
                spheresV1.resize(nbElements);
                bufferSize = nbElements * sizeof(SphereV1);
                file.read((char*)spheresV1.data(), bufferSize);
                for (uint64_t s = 0; s < spheresV1.size(); ++s)
                    spheres[s] = {spheresV1[i].center, spheresV1[i].radius};
            }
        }
        else
        {
            if (version >= CACHE_VERSION_2)
                bufferSize = nbElements * sizeof(Sphere);
            else
                bufferSize = nbElements * sizeof(SphereV1);
            file.ignore(bufferSize);
        }
    }

    // Cylinders
    file.read((char*)&nbCylinders, sizeof(size_t));
    for (size_t i = 0; i < nbCylinders; ++i)
    {
        file.read((char*)&materialId, sizeof(size_t));
        file.read((char*)&nbElements, sizeof(size_t));

        struct CylinderV1
        {
            Vector3f center;
            Vector3f up;
            float radius;
            float timestamp;
            float value;
        };

        if (props.getProperty<bool>(PROP_LOAD_CYLINDERS.name))
        {
            callback.updateProgress("Cylinders (" + std::to_string(i + 1) + "/" + std::to_string(nbCylinders) + ")",
                                    0.3f + 0.1f * float(i) / float(nbCylinders));
            auto& cylinders = model->getCylinders()[materialId];
            cylinders.resize(nbElements);
            if (version >= CACHE_VERSION_2)
            {
                bufferSize = nbElements * sizeof(Cylinder);
                file.read((char*)cylinders.data(), bufferSize);
            }
            else
            {
                std::vector<CylinderV1> cylindersV1(nbElements);
                bufferSize = nbElements * sizeof(CylinderV1);
                file.read((char*)cylindersV1.data(), bufferSize);
                for (uint64_t s = 0; s < cylindersV1.size(); ++s)
                    cylinders[s] = {cylindersV1[i].center, cylindersV1[i].up, cylindersV1[i].radius};
            }
        }
        else
        {
            if (version >= CACHE_VERSION_2)
                bufferSize = nbElements * sizeof(Cylinder);
            else
                bufferSize = nbElements * sizeof(CylinderV1);
            file.ignore(bufferSize);
        }
    }

    // Cones
    file.read((char*)&nbCones, sizeof(size_t));
    for (size_t i = 0; i < nbCones; ++i)
    {
        file.read((char*)&materialId, sizeof(size_t));
        file.read((char*)&nbElements, sizeof(size_t));

        struct ConeV1
        {
            Vector3f center;
            Vector3f up;
            float centerRadius;
            float upRadius;
            float timestamp;
            float value;
        };

        if (props.getProperty<bool>(PROP_LOAD_CONES.name))
        {
            callback.updateProgress("Cones (" + std::to_string(i + 1) + "/" + std::to_string(nbCones) + ")",
                                    0.4f + 0.1f * float(i) / float(nbCones));
            auto& cones = model->getCones()[materialId];
            cones.resize(nbElements);
            if (version >= CACHE_VERSION_2)
            {
                bufferSize = nbElements * sizeof(Cone);
                file.read((char*)cones.data(), bufferSize);
            }
            else
            {
                std::vector<ConeV1> conesV1(nbElements);
                bufferSize = nbElements * sizeof(ConeV1);
                file.read((char*)conesV1.data(), bufferSize);
                for (uint64_t s = 0; s < conesV1.size(); ++s)
                    cones[s] = {conesV1[i].center, conesV1[i].up, conesV1[i].centerRadius, conesV1[i].upRadius};
            }
        }
        else
        {
            if (version >= CACHE_VERSION_2)
                bufferSize = nbElements * sizeof(Cone);
            else
                bufferSize = nbElements * sizeof(ConeV1);

            file.ignore(bufferSize);
        }
    }

    // Meshes
    bool load = props.getProperty<bool>(PROP_LOAD_MESHES.name);
    file.read((char*)&nbMeshes, sizeof(size_t));
    for (size_t i = 0; i < nbMeshes; ++i)
    {
        file.read((char*)&materialId, sizeof(size_t));
        auto& meshes = model->getTriangleMeshes()[materialId];
        // Vertices
        file.read((char*)&nbVertices, sizeof(size_t));
        if (nbVertices != 0)
        {
            bufferSize = nbVertices * sizeof(Vector3f);
            if (load)
            {
                callback.updateProgress("Meshes (" + std::to_string(i + 1) + "/" + std::to_string(nbMeshes) + ")",
                                        0.5f + 0.1f * float(i) / float(nbMeshes));
                meshes.vertices.resize(nbVertices);
                file.read((char*)meshes.vertices.data(), bufferSize);
            }
            else
                file.ignore(bufferSize);
        }

        // Indices
        file.read((char*)&nbIndices, sizeof(size_t));
        if (nbIndices != 0)
        {
            bufferSize = nbIndices * sizeof(Vector3ui);
            if (load)
            {
                meshes.indices.resize(nbIndices);
                file.read((char*)meshes.indices.data(), bufferSize);
            }
            else
                file.ignore(bufferSize);
        }

        // Normals
        file.read((char*)&nbNormals, sizeof(size_t));
        if (nbNormals != 0)
        {
            bufferSize = nbNormals * sizeof(Vector3f);
            if (load)
            {
                meshes.normals.resize(nbNormals);
                file.read((char*)meshes.normals.data(), bufferSize);
            }
            else
                file.ignore(bufferSize);
        }

        // Texture coordinates
        file.read((char*)&nbTexCoords, sizeof(size_t));
        if (nbTexCoords != 0)
        {
            bufferSize = nbTexCoords * sizeof(Vector2f);
            if (load)
            {
                meshes.textureCoordinates.resize(nbTexCoords);
                file.read((char*)meshes.textureCoordinates.data(), bufferSize);
            }
            else
                file.ignore(bufferSize);
        }
    }

    // Streamlines
    load = props.getProperty<bool>(PROP_LOAD_STREAMLINES.name);
    size_t nbStreamlines;
    auto& streamlines = model->getStreamlines();
    file.read((char*)&nbStreamlines, sizeof(size_t));
    for (size_t i = 0; i < nbStreamlines; ++i)
    {
        StreamlinesData streamlineData;
        // Id
        size_t id;
        file.read((char*)&id, sizeof(size_t));

        // Vertex
        file.read((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector4f);
        if (load)
        {
            callback.updateProgress("Streamlines (" + std::to_string(i + 1) + "/" + std::to_string(nbStreamlines) + ")",
                                    0.6f + 0.1f * float(i) / float(nbStreamlines));
            streamlineData.vertex.resize(nbElements);
            file.read((char*)streamlineData.vertex.data(), bufferSize);
        }
        else
            file.ignore(bufferSize);

        // Vertex Color
        file.read((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector4f);
        if (load)
        {
            streamlineData.vertexColor.resize(nbElements);
            file.read((char*)streamlineData.vertexColor.data(), bufferSize);
        }
        else
            file.ignore(bufferSize);

        // Indices
        file.read((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(int32_t);
        if (load)
        {
            streamlineData.indices.resize(nbElements);
            file.read((char*)streamlineData.indices.data(), bufferSize);
        }
        else
            file.ignore(bufferSize);

        streamlines[id] = streamlineData;
    }

    // SDF geometry
    load = props.getProperty<bool>(PROP_LOAD_SDF.name);
    file.read((char*)&nbElements, sizeof(size_t));

    if (nbElements > 0)
    {
        // Geometries
        auto& sdfData = model->getSDFGeometryData();
        sdfData.geometries.resize(nbElements);
        bufferSize = nbElements * sizeof(SDFGeometry);
        file.read((char*)sdfData.geometries.data(), bufferSize);

        if (version <= CACHE_VERSION_3)
            // Update userParams (Displacement parameters)
            for (uint64_t i = 0; i < nbElements; ++i)
            {
                const char* index = (char*)sdfData.geometries.data() + i * sizeof(SDFGeometry) + sizeof(uint64_t);
                memcpy((char*)index, &neuroscience::common::DISPLACEMENT_PARAMS, sizeof(Vector3f));
            }

        // SDF Indices
        file.read((char*)&nbElements, sizeof(size_t));
        for (size_t i = 0; i < nbElements; ++i)
        {
            file.read((char*)&materialId, sizeof(size_t));
            size_t size;
            file.read((char*)&size, sizeof(size_t));
            bufferSize = size * sizeof(uint64_t);
            if (load)
            {
                callback.updateProgress("SDF geometries indices (" + std::to_string(i + 1) + "/" +
                                            std::to_string(nbElements) + ")",
                                        0.8f + 0.1f * float(i) / float(nbElements));
                sdfData.geometryIndices[materialId].resize(size);
                file.read((char*)sdfData.geometryIndices[materialId].data(), bufferSize);
            }
            else
                file.ignore(bufferSize);
        }

        // Neighbours
        file.read((char*)&nbElements, sizeof(size_t));
        sdfData.neighbours.resize(nbElements);

        if (load)
            callback.updateProgress("SDF geometries neighbours", 0.9f);

        for (size_t i = 0; i < nbElements; ++i)
        {
            size_t size;
            file.read((char*)&size, sizeof(size_t));
            bufferSize = size * sizeof(uint64_t);
            if (load)
            {
                sdfData.neighbours[i].resize(size);
                file.read((char*)sdfData.neighbours[i].data(), bufferSize);
            }
            else
                file.ignore(bufferSize);
        }

        // Neighbours flat
        file.read((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(uint64_t);
        if (load)
        {
            sdfData.neighboursFlat.resize(nbElements);
            file.read((char*)sdfData.neighboursFlat.data(), bufferSize);
        }
        else
            file.ignore(bufferSize);
    }

    load = props.getProperty<bool>(PROP_LOAD_SIMULATION.name);
    if (version >= CACHE_VERSION_3 && load)
    {
        // Simulation Handler
        size_t reportType{0};
        file.read((char*)&reportType, sizeof(size_t));

        switch (static_cast<neuroscience::common::ReportType>(reportType))
        {
        case neuroscience::common::ReportType::voltages_from_file:
        {
            // Report path
            const auto reportPath = _readString(file);

            // GIDs
            file.read((char*)&nbElements, sizeof(size_t));
            brion::GIDSet gids;
            for (uint32_t i = 0; i < nbElements; ++i)
            {
                uint32_t gid;
                file.read((char*)&gid, sizeof(uint32_t));
                gids.insert(gid);
            }

            // Synchronization
            bool synchronized{false};
            file.read((char*)&synchronized, sizeof(bool));

            // Handler
            auto handler = std::make_shared<VoltageSimulationHandler>(reportPath, gids, synchronized);
            model->setSimulationHandler(handler);
            break;
        }
        case neuroscience::common::ReportType::spikes:
        {
            // Report path
            const auto reportPath = _readString(file);

            // GIDs
            file.read((char*)&nbElements, sizeof(size_t));
            brion::GIDSet gids;
            for (uint32_t i = 0; i < nbElements; ++i)
            {
                uint32_t gid;
                file.read((char*)&gid, sizeof(uint32_t));
                gids.insert(gid);
            }

            // Handler
            auto handler = std::make_shared<SpikeSimulationHandler>(reportPath, gids);
            model->setSimulationHandler(handler);
            break;
        }
        default:
        {
            // No report in that brick!
        }
        }

        // Transfer function
        file.read((char*)&nbElements, sizeof(size_t));
        if (nbElements == 1)
        {
            auto& tf = model->getTransferFunction();
            // Values range
            Vector2d valuesRange;
            file.read((char*)&valuesRange, sizeof(Vector2d));
            tf.setValuesRange(valuesRange);

            // Control points
            file.read((char*)&nbElements, sizeof(size_t));
            Vector2ds controlPoints(nbElements);
            file.read((char*)&controlPoints[0], nbElements * sizeof(Vector2d));
            tf.setControlPoints(controlPoints);

            // Color map
            ColorMap colorMap;
            colorMap.name = _readString(file);
            file.read((char*)&nbElements, sizeof(size_t));
            auto& colors = colorMap.colors;
            colors.resize(nbElements);
            file.read((char*)&colors[0], nbElements * sizeof(Vector3f));
            tf.setColorMap(colorMap);
        }
    }
    callback.updateProgress("Done", 1.f);

    file.close();

    // Restore original circuit config file from cache metadata, if present
    std::string circuitPath = path;
    auto cpIt = metadata.find("CircuitPath");
    if (cpIt != metadata.end())
        circuitPath = cpIt->second;

    auto modelDescriptor = std::make_shared<ModelDescriptor>(std::move(model), "Brick", circuitPath, metadata);
    return modelDescriptor;
}

void SonataCacheLoader::exportToFile(const ModelDescriptorPtr modelDescriptor, const std::string& filename)
{
    PLUGIN_INFO("Saving model to cache file: " << filename);
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file.good())
    {
        const std::string msg = "Could not open cache file " + filename;
        PLUGIN_THROW(msg);
    }

    const size_t version = CACHE_VERSION_3;
    file.write((char*)&version, sizeof(size_t));

    // Save geometry
    auto& model = modelDescriptor->getModel();
    uint64_t bufferSize{0};

    // Metadata
    auto metadata = modelDescriptor->getMetadata();
    size_t nbElements = metadata.size();
    file.write((char*)&nbElements, sizeof(size_t));
    for (const auto& data : metadata)
    {
        size_t size = data.first.length();
        file.write((char*)&size, sizeof(size_t));
        file.write((char*)data.first.c_str(), size);
        size = data.second.length();
        file.write((char*)&size, sizeof(size_t));
        file.write((char*)data.second.c_str(), size);
    }

    const auto& materials = model.getMaterials();
    const auto nbMaterials = materials.size();
    file.write((char*)&nbMaterials, sizeof(size_t));

    // Save materials
    for (const auto& material : materials)
    {
        try
        {
            file.write((char*)&material.first, sizeof(size_t));

            auto name = material.second->getName();
            size_t size = name.length();
            file.write((char*)&size, sizeof(size_t));
            file.write((char*)name.c_str(), size);

            Vector3f value3f;
            value3f = material.second->getDiffuseColor();
            file.write((char*)&value3f, sizeof(Vector3f));
            value3f = material.second->getSpecularColor();
            file.write((char*)&value3f, sizeof(Vector3f));
            float value = material.second->getSpecularExponent();
            file.write((char*)&value, sizeof(float));
            value = material.second->getReflectionIndex();
            file.write((char*)&value, sizeof(float));
            value = material.second->getOpacity();
            file.write((char*)&value, sizeof(float));
            value = material.second->getRefractionIndex();
            file.write((char*)&value, sizeof(float));
            value = material.second->getEmission();
            file.write((char*)&value, sizeof(float));
            value = material.second->getGlossiness();
            file.write((char*)&value, sizeof(float));
            int32_t castUserData = material.second->getCastUserData();
            file.write((char*)&castUserData, sizeof(int32_t));
            int32_t shadingMode = material.second->getShadingMode();
            file.write((char*)&shadingMode, sizeof(int32_t));
        }
        catch (const std::runtime_error&)
        {
        }

        // TODO: Change bool to int32_t for Version 4
        bool clipped = false;
        try
        {
            clipped = material.second->getProperty<bool>(MATERIAL_PROPERTY_CLIPPING_MODE);
        }
        catch (const std::runtime_error&)
        {
        }
        file.write((char*)&clipped, sizeof(bool));
    }

    // Spheres
    nbElements = model.getSpheres().size();
    file.write((char*)&nbElements, sizeof(size_t));
    for (auto& spheres : model.getSpheres())
    {
        const auto materialId = spheres.first;
        file.write((char*)&materialId, sizeof(size_t));

        const auto& data = spheres.second;
        nbElements = data.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Sphere);
        file.write((char*)data.data(), bufferSize);
    }

    // Cylinders
    nbElements = model.getCylinders().size();
    file.write((char*)&nbElements, sizeof(size_t));
    for (auto& cylinders : model.getCylinders())
    {
        const auto materialId = cylinders.first;
        file.write((char*)&materialId, sizeof(size_t));

        const auto& data = cylinders.second;
        nbElements = data.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Cylinder);
        file.write((char*)data.data(), bufferSize);
    }

    // Cones
    nbElements = model.getCones().size();
    file.write((char*)&nbElements, sizeof(size_t));
    for (auto& cones : model.getCones())
    {
        const auto materialId = cones.first;
        file.write((char*)&materialId, sizeof(size_t));

        const auto& data = cones.second;
        nbElements = data.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Cone);
        file.write((char*)data.data(), bufferSize);
    }

    // Meshes
    nbElements = model.getTriangleMeshes().size();
    file.write((char*)&nbElements, sizeof(size_t));
    for (const auto& meshes : model.getTriangleMeshes())
    {
        const auto materialId = meshes.first;
        file.write((char*)&materialId, sizeof(size_t));

        const auto& data = meshes.second;

        // Vertices
        nbElements = data.vertices.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector3f);
        file.write((char*)data.vertices.data(), bufferSize);

        // Indices
        nbElements = data.indices.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector3ui);
        file.write((char*)data.indices.data(), bufferSize);

        // Normals
        nbElements = data.normals.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector3f);
        file.write((char*)data.normals.data(), bufferSize);

        // Texture coordinates
        nbElements = data.textureCoordinates.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector2f);
        file.write((char*)data.textureCoordinates.data(), bufferSize);
    }

    // Streamlines
    const auto& streamlines = model.getStreamlines();
    nbElements = streamlines.size();
    file.write((char*)&nbElements, sizeof(size_t));
    for (const auto& streamline : streamlines)
    {
        const auto& streamlineData = streamline.second;
        // Id
        size_t id = streamline.first;
        file.write((char*)&id, sizeof(size_t));

        // Vertex
        nbElements = streamlineData.vertex.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector4f);
        file.write((char*)streamlineData.vertex.data(), bufferSize);

        // Vertex Color
        nbElements = streamlineData.vertexColor.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(Vector4f);
        file.write((char*)streamlineData.vertexColor.data(), bufferSize);

        // Indices
        nbElements = streamlineData.indices.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(int32_t);
        file.write((char*)streamlineData.indices.data(), bufferSize);
    }

    // SDF geometry
    const auto& sdfData = model.getSDFGeometryData();
    nbElements = sdfData.geometries.size();
    file.write((char*)&nbElements, sizeof(size_t));

    if (nbElements > 0)
    {
        // Geometries
        bufferSize = nbElements * sizeof(SDFGeometry);
        file.write((char*)sdfData.geometries.data(), bufferSize);

        // SDF indices
        nbElements = sdfData.geometryIndices.size();
        file.write((char*)&nbElements, sizeof(size_t));
        for (const auto& geometryIndex : sdfData.geometryIndices)
        {
            size_t materialId = geometryIndex.first;
            file.write((char*)&materialId, sizeof(size_t));
            nbElements = geometryIndex.second.size();
            file.write((char*)&nbElements, sizeof(size_t));
            bufferSize = nbElements * sizeof(uint64_t);
            file.write((char*)geometryIndex.second.data(), bufferSize);
        }

        // Neighbours
        nbElements = sdfData.neighbours.size();
        file.write((char*)&nbElements, sizeof(size_t));
        for (const auto& neighbour : sdfData.neighbours)
        {
            nbElements = neighbour.size();
            file.write((char*)&nbElements, sizeof(size_t));
            bufferSize = nbElements * sizeof(size_t);
            file.write((char*)neighbour.data(), bufferSize);
        }

        // Neighbours flat
        nbElements = sdfData.neighboursFlat.size();
        file.write((char*)&nbElements, sizeof(size_t));
        bufferSize = nbElements * sizeof(uint64_t);
        file.write((char*)sdfData.neighboursFlat.data(), bufferSize);
    }

    // Simulation handler
    const AbstractSimulationHandlerPtr handler = model.getSimulationHandler();
    if (handler)
    {
        VoltageSimulationHandler* vsh = dynamic_cast<VoltageSimulationHandler*>(handler.get());
        SpikeSimulationHandler* ssh = dynamic_cast<SpikeSimulationHandler*>(handler.get());
        if (vsh)
        {
            const size_t reportType{static_cast<size_t>(neuroscience::common::ReportType::voltages_from_file)};
            file.write((char*)&reportType, sizeof(size_t));

            // Report path
            const auto& value = vsh->getReportPath();
            size_t size = value.length();
            file.write((char*)&size, sizeof(size_t));
            file.write((char*)value.c_str(), size);

            // Gids
            const brion::GIDSet& gids = vsh->getReport()->getGIDs();
            size = gids.size();
            file.write((char*)&size, sizeof(size_t));
            for (const auto gid : gids)
                file.write((char*)&gid, sizeof(uint32_t));

            // Synchronization mode
            const bool sync = vsh->isSynchronized();
            file.write((char*)&sync, sizeof(bool));
        }
        else if (ssh)
        {
            const size_t reportType{static_cast<size_t>(neuroscience::common::ReportType::spikes)};
            file.write((char*)&reportType, sizeof(size_t));

            // Report path
            const auto& value = ssh->getReportPath();
            size_t size = value.length();
            file.write((char*)&size, sizeof(size_t));
            file.write((char*)value.c_str(), size);

            // Gids
            const brion::GIDSet& gids = ssh->getGIDs();
            size = gids.size();
            file.write((char*)&size, sizeof(size_t));
            for (const auto gid : gids)
                file.write((char*)&gid, sizeof(uint32_t));
        }
        else
        {
            // Handler is ignored. Only voltage simulation handler is
            // currently supported
            const size_t reportType{static_cast<size_t>(neuroscience::common::ReportType::undefined)};
            file.write((char*)&reportType, sizeof(size_t));
        }
    }
    else
    {
        // No handler
        nbElements = 0;
        file.write((char*)&nbElements, sizeof(size_t));
    }

    // Transfer function
    nbElements = 1;
    file.write((char*)&nbElements, sizeof(size_t));
    const auto& tf = model.getTransferFunction();
    {
        // Values range
        const Vector2d& valuesRange = tf.getValuesRange();
        file.write((char*)&valuesRange, sizeof(Vector2d));

        // Control points
        const Vector2ds& controlPoints = tf.getControlPoints();
        nbElements = controlPoints.size();
        file.write((char*)&nbElements, sizeof(size_t));
        file.write((char*)&controlPoints[0], nbElements * sizeof(Vector2d));

        // Color map
        const ColorMap& colorMap = tf.getColorMap();
        const std::string name = colorMap.name;
        const size_t size = name.length();
        file.write((char*)&size, sizeof(size_t));
        file.write((char*)name.c_str(), size);
        nbElements = colorMap.colors.size();
        file.write((char*)&nbElements, sizeof(size_t));
        file.write((char*)&colorMap.colors[0], nbElements * sizeof(Vector3f));
    }

    file.close();
}

PropertyMap SonataCacheLoader::getProperties() const
{
    return _defaults;
}

PropertyMap SonataCacheLoader::getCLIProperties()
{
    PropertyMap pm(LOADER_NAME);
    pm.setProperty(PROP_LOAD_SPHERES);
    pm.setProperty(PROP_LOAD_CYLINDERS);
    pm.setProperty(PROP_LOAD_CONES);
    pm.setProperty(PROP_LOAD_MESHES);
    pm.setProperty(PROP_LOAD_STREAMLINES);
    pm.setProperty(PROP_LOAD_SDF);
    pm.setProperty(PROP_LOAD_SIMULATION);
    return pm;
}
} // namespace loader
} // namespace io
} // namespace sonataexplorer

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

#include "Scene.h"

#include <platform/core/common/Logs.h>
#include <platform/core/common/Transformation.h>
#include <platform/core/common/scene/ClipPlane.h>
#include <platform/core/common/utils/Utils.h>
#include <platform/core/engineapi/Material.h>
#include <platform/core/engineapi/Model.h>

#include <platform/core/parameters/GeometryParameters.h>

#include <platform/core/common/utils/FileSystem.h>

#include <fstream>
#include <mutex>

namespace
{
template <typename T, typename U = T> // U seems to be needed when getID is a
                                      // member function of a base of T.
std::shared_ptr<T> _find(const std::vector<std::shared_ptr<T>>& list, const size_t id,
                         size_t (U::*getID)() const = &T::getID)
{
    auto i = std::find_if(list.begin(), list.end(), [id, getID](auto x) { return id == ((*x).*getID)(); });
    return i == list.end() ? std::shared_ptr<T>{} : *i;
}

template <typename T, typename U = T>
std::shared_ptr<T> _remove(std::vector<std::shared_ptr<T>>& list, const size_t id,
                           size_t (U::*getID)() const = &T::getID)
{
    auto i = std::find_if(list.begin(), list.end(), [id, getID](auto x) { return id == ((*x).*getID)(); });
    if (i == list.end())
        return std::shared_ptr<T>{};
    auto result = *i;
    list.erase(i);
    return result;
}
} // namespace

namespace core
{
Scene::Scene(AnimationParameters& animationParameters, GeometryParameters& geometryParameters,
             VolumeParameters& volumeParameters, FieldParameters& fieldParameters)
    : _animationParameters(animationParameters)
    , _geometryParameters(geometryParameters)
    , _volumeParameters(volumeParameters)
    , _fieldParameters(fieldParameters)
{
}

void Scene::copyFrom(const Scene& rhs)
{
    if (this == &rhs)
        return;

    {
        std::unique_lock<std::shared_timed_mutex> lock(_modelMutex);
        std::shared_lock<std::shared_timed_mutex> rhsLock(rhs._modelMutex);

        _modelDescriptors.clear();
        _modelDescriptors.reserve(rhs._modelDescriptors.size());
        for (const auto& modelDesc : rhs._modelDescriptors)
            _modelDescriptors.push_back(modelDesc->clone(createModel()));
    }
    computeBounds();

    *_backgroundMaterial = *rhs._backgroundMaterial;
    _backgroundMaterial->markModified();

    _lightManager = rhs._lightManager;
    _clipPlanes = rhs._clipPlanes;

    markModified();
}

void Scene::commit() {}

size_t Scene::getSizeInBytes() const
{
    auto lock = acquireReadAccess();
    size_t sizeInBytes = 0;
    for (auto modelDescriptor : _modelDescriptors)
        sizeInBytes += modelDescriptor->getModel().getSizeInBytes();
    return sizeInBytes;
}

size_t Scene::getNumModels() const
{
    auto lock = acquireReadAccess();
    return _modelDescriptors.size();
}

size_t Scene::addModel(ModelDescriptorPtr modelDescriptor)
{
    auto& model = modelDescriptor->getModel();
    if (model.empty())
        throw std::runtime_error("Empty models not supported.");

    const auto defaultBVHFlags = _geometryParameters.getDefaultBVHFlags();

    model.setBVHFlags(defaultBVHFlags);
    model.buildBoundingBox();

    // Since models can be added concurrently we check if that is supported
    if (supportsConcurrentSceneUpdates())
        model.commitGeometry();

    {
        std::unique_lock<std::shared_timed_mutex> lock(_modelMutex);
        modelDescriptor->setModelID(_modelID++);
        _modelDescriptors.push_back(modelDescriptor);

        // add default instance of this model to render something
        if (modelDescriptor->getInstances().empty())
            modelDescriptor->addInstance({true, true, modelDescriptor->getTransformation()});
    }

    computeBounds();
    markModified();

    return modelDescriptor->getModelID();
}

bool Scene::removeModel(const size_t id)
{
    ModelDescriptorPtr model = nullptr;

    if (supportsConcurrentSceneUpdates())
    {
        {
            std::unique_lock<std::shared_timed_mutex> lock(_modelMutex);
            model = _remove(_modelDescriptors, id, &ModelDescriptor::getModelID);
        }
        if (model)
            model->callOnRemoved();
    }
    else
    {
        {
            std::unique_lock<std::shared_timed_mutex> lock(_modelMutex);
            model = _find(_modelDescriptors, id, &ModelDescriptor::getModelID);
        }
        if (model)
            model->markForRemoval();
    }

    if (model)
    {
        markModified();
        return true;
    }

    return false;
}

ModelDescriptorPtr Scene::getModel(const size_t id) const
{
    auto lock = acquireReadAccess();
    return _find(_modelDescriptors, id, &ModelDescriptor::getModelID);
}

bool Scene::empty() const
{
    auto lock = acquireReadAccess();
    for (auto modelDescriptor : _modelDescriptors)
        if (!modelDescriptor->getModel().empty())
            return false;
    return true;
}

size_t Scene::addClipPlane(const Plane& plane)
{
    auto clipPlane = std::make_shared<ClipPlane>(plane);
    clipPlane->onModified([&](const BaseObject&) { markModified(false); });
    _clipPlanes.emplace_back(std::move(clipPlane));
    markModified();
    return _clipPlanes.back()->getID();
}

ClipPlanePtr Scene::getClipPlane(const size_t id) const
{
    return _find(_clipPlanes, id);
}

void Scene::removeClipPlane(const size_t id)
{
    if (_remove(_clipPlanes, id))
        markModified();
}

ModelDescriptorPtr Scene::loadModel(Blob&& blob, const ModelParams& params, LoaderProgress cb)
{
    const auto& loader = _loaderRegistry.getSuitableLoader("", blob.type, params.getLoaderName());

    // HACK: Add loader name in properties for archive loader
    auto propCopy = params.getLoaderProperties();
    propCopy.setProperty({"loaderName", params.getLoaderName()});
    auto modelDescriptor = loader.importFromBlob(std::move(blob), cb, propCopy);
    if (!modelDescriptor)
        throw std::runtime_error("No model returned by loader");
    *modelDescriptor = params;
    addModel(modelDescriptor);
    return modelDescriptor;
}

ModelDescriptorPtr Scene::loadModel(const std::string& path, const ModelParams& params, LoaderProgress cb)
{
    const auto& loader = _loaderRegistry.getSuitableLoader(path, "", params.getLoaderName());
    // HACK: Add loader name in properties for archive loader
    auto propCopy = params.getLoaderProperties();
    propCopy.setProperty({"loaderName", params.getLoaderName()});
    auto modelDescriptor = loader.importFromStorage(path, cb, propCopy);
    if (!modelDescriptor)
        throw std::runtime_error("No model returned by loader");
    *modelDescriptor = params;
    addModel(modelDescriptor);
    return modelDescriptor;
}

void Scene::visitModels(const std::function<void(Model&)>& functor)
{
    std::unique_lock<std::shared_timed_mutex> lock(_modelMutex);
    for (const auto& modelDescriptor : _modelDescriptors)
        functor(modelDescriptor->getModel());
}

void Scene::buildDefault()
{
    CORE_INFO("Building default Cornell Box scene");

    auto model = createModel();
#if 0
    const Vector3f WHITE = {1.f, 1.f, 1.f};

    const Vector3f positions[8] = {
        {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, //    6--------7
        {0.f, 1.f, 0.f},                  //   /|       /|
        {1.f, 1.f, 0.f},                  //  2--------3 |
        {0.f, 0.f, 1.f},                  //  | |      | |
        {1.f, 0.f, 1.f},                  //  | 4------|-5
        {0.f, 1.f, 1.f},                  //  |/       |/
        {1.f, 1.f, 1.f}                   //  0--------1
    };

    const uint16_t indices[6][6] = {
        {5, 4, 6, 6, 7, 5}, // Front
        {7, 5, 1, 1, 3, 7}, // Right
        {3, 1, 0, 0, 2, 3}, // Back
        {2, 0, 4, 4, 6, 2}, // Left
        {0, 1, 5, 5, 4, 0}, // Bottom
        {7, 3, 2, 2, 6, 7}  // Top
    };

    const Vector3f colors[6] = {{0.8f, 0.8f, 0.8f}, {1.f, 0.f, 0.f},    {0.8f, 0.8f, 0.8f},
                                {0.f, 1.f, 0.f},    {0.8f, 0.8f, 0.8f}, {0.8f, 0.8f, 0.8f}};

    size_t materialId = 0;
    for (size_t i = 1; i < 6; ++i)
    {
        // Cornell box
        auto material = model->createMaterial(materialId, "wall_" + std::to_string(materialId));
        material->setDiffuseColor(colors[i]);
        material->setSpecularColor(WHITE);
        material->setSpecularExponent(10.f);
        material->setReflectionIndex(i == 4 ? 0.2f : 0.f);
        material->setGlossiness(i == 4 ? 0.9f : 1.f);
        material->setOpacity(1.f);

        auto& triangleMesh = model->getTriangleMeshes()[materialId];
        for (size_t j = 0; j < 6; ++j)
        {
            const auto position = positions[indices[i][j]];
            triangleMesh.vertices.push_back(position);
        }
        triangleMesh.indices.push_back(Vector3ui(0, 1, 2));
        triangleMesh.indices.push_back(Vector3ui(3, 4, 5));
        ++materialId;
    }

    {
        // Sphere
        auto material = model->createMaterial(materialId, "sphere");
        material->setOpacity(0.5f);
        material->setRefractionIndex(1.05f);
        material->setReflectionIndex(0.1f);
        material->setDiffuseColor(WHITE);
        material->setSpecularColor(WHITE);
        material->setSpecularExponent(100.f);
        material->setShadingMode(MaterialShadingMode::diffuse);
        model->addSphere(materialId, {{0.25f, 0.26f, 0.30f}, 0.25f});
        ++materialId;
    }

    {
        // Cylinder
        auto material = model->createMaterial(materialId, "cylinder");
        material->setDiffuseColor({0.1f, 0.1f, 0.8f});
        material->setSpecularColor(WHITE);
        material->setSpecularExponent(10.f);
        material->setShadingMode(MaterialShadingMode::diffuse);
        model->addCylinder(materialId, {{0.25f, 0.126f, 0.75f}, {0.75f, 0.126f, 0.75f}, 0.125f});
        ++materialId;
    }

    {
        // Cone
        auto material = model->createMaterial(materialId, "cone");
        material->setReflectionIndex(0.8f);
        material->setSpecularColor(WHITE);
        material->setSpecularExponent(10.f);
        material->setShadingMode(MaterialShadingMode::diffuse);
        model->addCone(materialId, {{0.75f, 0.01f, 0.25f}, {0.75f, 0.5f, 0.25f}, 0.15f, 0.f});
        ++materialId;
    }

/*
    {
        // Lamp
        auto material = model->createMaterial(materialId, "lamp");
        material->setDiffuseColor(WHITE);
        material->setEmission(5.f);
        const Vector3f lampInfo = {0.15f, 0.99f, 0.15f};
        const Vector3f lampPositions[4] = {{0.5f - lampInfo.x, lampInfo.y, 0.5f - lampInfo.z},
                                           {0.5f + lampInfo.x, lampInfo.y, 0.5f - lampInfo.z},
                                           {0.5f + lampInfo.x, lampInfo.y, 0.5f + lampInfo.z},
                                           {0.5f - lampInfo.x, lampInfo.y, 0.5f + lampInfo.z}};
        auto& triangleMesh = model->getTriangleMeshes()[materialId];
        for (size_t i = 0; i < 4; ++i)
            triangleMesh.vertices.push_back(lampPositions[i]);
        triangleMesh.indices.push_back(Vector3i(2, 1, 0));
        triangleMesh.indices.push_back(Vector3i(0, 3, 2));
    }
*/
#else
    for (size_t materialId = 0; materialId < 10; ++materialId)
    {
        auto material = model->createMaterial(materialId, "Material");
        material->setOpacity(0.75f);
        material->setRefractionIndex(1.5f);
        material->setReflectionIndex(0.5f);
        material->setDiffuseColor({materialId * 0.1f, 0.f, 1.f - materialId * 0.1f});
        material->setSpecularColor({1.f, 1.f, 1.f});
        material->setSpecularExponent(100.f);
        material->setShadingMode(MaterialShadingMode::basic);
        material->setEmission(materialId % 2 == 0 ? 0.5f : 0.f);
        material->setCastUserData(true);

        model->addSphere(materialId, {{-50.f + materialId * 10.f, 0.f, 0.f}, 5.f});
        model->addCylinder(materialId,
                           {{-50.f + materialId * 10.f, 0.f, 0.f}, {-50.f + materialId * 10.f, 20.f, 0.f}, 2.5f});
        model->addCone(materialId,
                       {{-50.f + materialId * 10.f, 0.f, 0.f}, {-50.f + materialId * 10.f, -20.f, 0.f}, 2.5f, 0.f});
    }
#endif

    addModel(std::make_shared<ModelDescriptor>(std::move(model), "Demo scene"));
}

void Scene::setMaterialsColorMap(MaterialsColorMap colorMap)
{
    {
        auto lock = acquireReadAccess();
        for (auto modelDescriptors : _modelDescriptors)
            modelDescriptors->getModel().setMaterialsColorMap(colorMap);
    }
    markModified();
}

bool Scene::setEnvironmentMap(const std::string& envMap)
{
    bool success = true;
    if (envMap.empty())
    {
        if (_backgroundMaterial)
            _backgroundMaterial->clearTextures();
    }
    else
    {
        try
        {
            _backgroundMaterial->setTexture(envMap, TextureType::diffuse);
        }
        catch (const std::runtime_error& e)
        {
            CORE_DEBUG("Cannot load environment map: " << e.what());
            _backgroundMaterial->clearTextures();
            success = false;
        }

        _loadIBLMaps(envMap);
    }

    _updateValue(_environmentMap, success ? envMap : "");
    if (_backgroundMaterial && _backgroundMaterial->isModified())
        markModified();
    return success;
}

bool Scene::hasEnvironmentMap() const
{
    return !_environmentMap.empty();
}

void Scene::computeBounds()
{
    std::unique_lock<std::shared_timed_mutex> lock(_modelMutex);
    _bounds.reset();
    for (const auto& modelDescriptor : _modelDescriptors)
    {
        const auto& modelBounds = modelDescriptor->getModel().getBounds();
        Transformation modelTransformation;
        modelTransformation.setTranslation(modelBounds.getCenter());

        const auto modelHalfSize = modelBounds.getSize() / 2.0;

        Transformation finalTransformation = modelTransformation * modelDescriptor->getTransformation();
        _bounds.merge(finalTransformation.getTranslation() - modelHalfSize);
        _bounds.merge(finalTransformation.getTranslation() + modelHalfSize);
        for (const auto& instance : modelDescriptor->getInstances())
        {
            finalTransformation = modelTransformation * instance.getTransformation();
            _bounds.merge(finalTransformation.getTranslation() - modelHalfSize);
            _bounds.merge(finalTransformation.getTranslation() + modelHalfSize);
        }
    }

    if (_bounds.isEmpty())
        // If no model is enabled. return empty bounding box
        _bounds.merge({0, 0, 0});
}

void Scene::_loadIBLMaps(const std::string& envMap)
{
    try
    {
        auto tex = _backgroundMaterial->getTexture(TextureType::diffuse);

        const auto path = fs::path(envMap).parent_path();
        const auto basename = (path / fs::path(envMap).stem()).string();

        const std::string irradianceMap = basename + IRRADIANCE_MAP + ".hdr";
        const std::string radianceMap = basename + RADIANCE_MAP + ".hdr";
        const std::string brdfLUT = basename + BRDF_LUT + ".hdr";

        if (fs::exists(irradianceMap) && fs::exists(radianceMap) && fs::exists(brdfLUT))
        {
            _backgroundMaterial->setTexture(irradianceMap, TextureType::irradiance);
            _backgroundMaterial->setTexture(radianceMap, TextureType::radiance);
            _backgroundMaterial->setTexture(brdfLUT, TextureType::brdf_lut);
        }
    }
    catch (...)
    {
    }
}
} // namespace core

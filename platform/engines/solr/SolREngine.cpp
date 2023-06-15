/* Copyright (c) 2015-2016, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
 *
 * This file is part of Brayns <https://github.com/BlueBrain/Brayns>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "SolREngine.h"
#include "Logs.h"
#include "SolRCamera.h"
#include "SolRContext.h"
#include "SolRFrameBuffer.h"
#include "SolRMaterial.h"
#include "SolRRenderer.h"
#include "SolRScene.h"

#include <platform/core/common/input/KeyboardHandler.h>
#include <platform/core/parameters/ParametersManager.h>

namespace core
{
using namespace solr;

SolREngine::SolREngine(ParametersManager& parametersManager)
    : Engine(parametersManager)
{
    PLUGIN_INFO("Initializing scene");
    _scene = createScene(_parametersManager.getAnimationParameters(), _parametersManager.getGeometryParameters(),
                         _parametersManager.getVolumeParameters());

    PLUGIN_INFO("Initializing camera");
    _createCameras();

    PLUGIN_INFO("Initializing renderers");
    _createRenderers();

    PLUGIN_INFO("Initializing frame buffer");

    const Vector2ui frameSize = _parametersManager.getApplicationParameters().getWindowSize();

    auto kernel = SolRContext::get().getKernel();
    auto& sceneInfo = kernel->getSceneInfo();
    sceneInfo.size = {static_cast<int>(frameSize.x), static_cast<int>(frameSize.y)};

    PLUGIN_INFO("Engine initialization complete");
}

SolREngine::~SolREngine() {}

void SolREngine::commit()
{
    Engine::commit();
}

void SolREngine::preRender() {}

void SolREngine::_createRenderers()
{
    auto& rp = _parametersManager.getRenderingParameters();
    auto solrRenderer = std::make_shared<SolRRenderer>(_parametersManager.getAnimationParameters(), rp);

    for (const auto& renderer : rp.getRenderers())
    {
        PropertyMap properties;
        if (renderer == "basic")
        {
            properties.setProperty({"shadows", 0.0, 0.0, 1.0, {"Shadow intensity"}});
            properties.setProperty({"softShadows", 0.0, 0.0, 1.0, {"Shadow softness"}});
        }
    }
    _renderer = solrRenderer;
}

FrameBufferPtr SolREngine::createFrameBuffer(const std::string& name, const Vector2ui& frameSize,
                                             FrameBufferFormat frameBufferFormat) const
{
    auto frameBuffer = std::make_shared<SolRFrameBuffer>(name, frameSize, frameBufferFormat);

    auto kernel = SolRContext::get().getKernel();
    kernel->initBuffers();
    kernel->resetAll();
    kernel->setFrame(0);
    return frameBuffer;
}

ScenePtr SolREngine::createScene(AnimationParameters& animationParameters, GeometryParameters& geometryParameters,
                                 VolumeParameters& volumeParameters) const
{
    return std::make_shared<SolRScene>(animationParameters, geometryParameters, volumeParameters);
}

CameraPtr SolREngine::createCamera() const
{
    return std::make_shared<SolRCamera>();
}

void SolREngine::_createCameras()
{
    auto solrCamera = std::make_shared<SolRCamera>();

    Property fovy{"fovy", 45., .1, 360., {"Field of view"}};
    Property aspect{"aspect", 1., {"Aspect ratio"}};
    aspect.markReadOnly();

    RenderingParameters& rp = _parametersManager.getRenderingParameters();
    for (const auto& camera : rp.getCameras())
    {
        PropertyMap properties;
        properties.setProperty(aspect);
        if (camera == "perspective" || camera == "clippedperspective")
        {
            properties.setProperty(fovy);
            properties.setProperty({"apertureRadius", 0., {"Aperture radius"}});
            properties.setProperty({"focusDistance", 1., {"Focus Distance"}});
        }
        if (camera == "orthographic")
        {
            properties.setProperty({"height", 1., {"Height"}});
        }
        solrCamera->setProperties(camera, properties);
        solrCamera->setCurrentType(camera);
    }
    _camera = solrCamera;
}

RendererPtr SolREngine::createRenderer(const AnimationParameters& animationParameters,
                                       const RenderingParameters& renderingParameters) const
{
    return std::make_shared<SolRRenderer>(animationParameters, renderingParameters);
}

Vector2ui SolREngine::getMinimumFrameSize() const
{
    return {64, 64};
}

extern "C" core::Engine* core_engine_create(int, const char**, core::ParametersManager& parametersManager)
{
    PLUGIN_INFO("");
    PLUGIN_INFO("   _|_|_|            _|  _|_|_|  ");
    PLUGIN_INFO(" _|          _|_|    _|  _|    _|");
    PLUGIN_INFO("   _|_|    _|    _|  _|  _|_|_|  ");
    PLUGIN_INFO("       _|  _|    _|  _|  _|    _|");
    PLUGIN_INFO(" _|_|_|      _|_|    _|  _|    _|");
    PLUGIN_INFO("");

    return new core::SolREngine(parametersManager);
}

} // namespace core

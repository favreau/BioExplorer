/* Copyright (c) 2015-2017, EPFL/Blue Brain Project
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

#include "SolRScene.h"
#include "Logs.h"
#include "SolRContext.h"
#include "SolRMaterial.h"
#include "SolRModel.h"
#include "SolRVolume.h"

#include <platform/core/common/ImageManager.h>
#include <platform/core/common/Transformation.h>
#include <platform/core/common/light/Light.h>
#include <platform/core/common/simulation/AbstractSimulationHandler.h>

#include <platform/core/engineapi/Model.h>

#include <platform/core/parameters/GeometryParameters.h>
#include <platform/core/parameters/ParametersManager.h>

#ifdef USE_CUDA
#include <engines/CudaKernel.h>
#else
#ifdef USE_OPENCL
#include <engines/OpenCLKernel.h>
#endif // USE_OPENCL
#endif // USE_CUDA

namespace core
{
SolRScene::SolRScene(AnimationParameters& animationParameters, GeometryParameters& geometryParameters,
                     VolumeParameters& volumeParameters)
    : Scene(animationParameters, geometryParameters, volumeParameters)
{
    _backgroundMaterial = std::make_shared<SolRMaterial>();
}

SolRScene::~SolRScene() {}

void SolRScene::commit()
{
    if (!isModified())
        return;

    for (auto modelDescriptor : _modelDescriptors)
    {
        if (!modelDescriptor->getEnabled())
            continue;

        modelDescriptor->getModel().commitGeometry();
        PLUGIN_DEBUG("Committing " << modelDescriptor->getName());
    }

    computeBounds();
}

bool SolRScene::commitLights()
{
    PLUGIN_WARN("SolRModel::commitLights not properly implemented");

    const size_t materialId = 0;
    auto model = createModel();
    auto material = model->createMaterial(materialId, "Default light");
    material->setEmission(1.f);
    material->commit();

    auto kernel = SolRContext::get().getKernel();
    auto id = kernel->addPrimitive(solr::ptSphere);
    kernel->setPrimitive(id, -10.f, 5.f, -10.f, 1.f, 0.f, 0.f, materialId);
    kernel->setPrimitiveIsMovable(id, false);

    return true;
}

ModelPtr SolRScene::createModel() const
{
    return std::make_unique<SolRModel>(_animationParameters, _volumeParameters);
}

} // namespace core

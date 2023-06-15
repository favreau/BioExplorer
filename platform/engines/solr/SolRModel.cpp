/* Copyright (c) 2015-2018, EPFL/Blue Brain Project
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

#include "SolRModel.h"
#include "Logs.h"
#include "SolRContext.h"
#include "SolREngine.h"
#include "SolRMaterial.h"
#include "SolRVolume.h"

#include <platform/core/engineapi/Material.h>
#include <platform/core/engineapi/Scene.h>

#ifdef USE_CUDA
#include <engines/CudaKernel.h>
#endif
#ifdef USE_OPENCL
#include <engines/OpenCLKernel.h>
#endif

namespace core
{
using namespace solr;

SolRModel::SolRModel(AnimationParameters& animationParameters, VolumeParameters& volumeParameters)
    : Model(animationParameters, volumeParameters)
{
}

SolRModel::~SolRModel() {}

void SolRModel::commitGeometry()
{
    auto kernel = SolRContext::get().getKernel();
    // Materials
    for (auto material : _materials)
        material.second->commit();

    // Spheres
    if (_spheresDirty)
    {
        for (const auto& spheres : _geometries->_spheres)
        {
            const auto material = spheres.first;
            for (const auto& s : spheres.second)
            {
                const auto id = kernel->addPrimitive(solr::ptSphere, true);
                const auto c = s.center;
                const auto r = s.radius;
                kernel->setPrimitive(id, c.x, c.y, c.z, r, 0, 0, material);
            }
        }
        _spheresDirty = false;
    }

    // Cylinders
    if (_cylindersDirty)
    {
        for (const auto& cylinders : _geometries->_cylinders)
        {
            const auto material = cylinders.first;
            for (const auto& c : cylinders.second)
            {
                const auto id = kernel->addPrimitive(solr::ptCylinder, true);
                const auto c0 = c.center;
                const auto c1 = c.center + c.up;
                const auto r = c.radius;
                kernel->setPrimitive(id, c0.x, c0.y, c0.z, c1.x, c1.y, c1.z, r, 0, 0, material);
            }
        }
        _cylindersDirty = false;
    }

    // Cones
    if (_conesDirty)
    {
        for (const auto& cones : _geometries->_cones)
        {
            const auto material = cones.first;
            for (const auto& c : cones.second)
            {
                const auto id = kernel->addPrimitive(solr::ptCone, true);
                const auto c0 = c.center;
                const auto c1 = c.center + c.up;
                const auto r0 = c.centerRadius;
                const auto r1 = c.upRadius;
                kernel->setPrimitive(id, c0.x, c0.y, c0.z, c1.x, c1.y, c1.z, r0, r1, 0, material);
            }
        }
        _conesDirty = false;
    }

    updateBounds();
    _markGeometriesClean();

    // handled by the scene
    _instancesDirty = false;

    kernel->compactBoxes(true);
}

MaterialPtr SolRModel::createMaterialImpl(const PropertyMap& properties)
{
    MaterialPtr material = std::make_shared<SolRMaterial>();
    if (!material)
        PLUGIN_THROW(std::runtime_error("Failed to create material"));
    return material;
}

void SolRModel::buildBoundingBox()
{
    PLUGIN_ERROR("SolRModel::buildBoundingBox not implemented");
}

SharedDataVolumePtr SolRModel::createSharedDataVolume(const Vector3ui& dimensions, const Vector3f& spacing,
                                                      const DataType type)
{
    PLUGIN_THROW("SolRModel::createSharedDataVolume not implemented");
}

BrickedVolumePtr SolRModel::createBrickedVolume(const Vector3ui& dimensions, const Vector3f& spacing,
                                                const DataType type)
{
    PLUGIN_THROW("SolRModel::createBrickedVolume not implemented");
}

void SolRModel::_commitTransferFunctionImpl(const Vector3fs& colors, const floats& opacities, const Vector2d valueRange)
{
    PLUGIN_THROW("SolRModel::_commitTransferFunctionImpl not implemented");
}

void SolRModel::_commitSimulationDataImpl(const float* frameData, const size_t frameSize)
{
    PLUGIN_THROW("SolRModel::_commitSimulationDataImpl not implemented");
}

} // namespace core

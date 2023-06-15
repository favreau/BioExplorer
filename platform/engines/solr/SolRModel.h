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

#pragma once

#include "SolRTypes.h"

#include <platform/core/engineapi/Model.h>
#include <platform/core/parameters/ParametersManager.h>

namespace core
{
using namespace solr;

class SolRModel : public Model
{
public:
    SolRModel(AnimationParameters& animationParameters, VolumeParameters& volumeParameters);
    ~SolRModel() final;

    MaterialPtr createMaterialImpl(const PropertyMap& properties = {}) final;

    void buildBoundingBox() final;

    void commitGeometry() final;

    SharedDataVolumePtr createSharedDataVolume(const Vector3ui& dimensions, const Vector3f& spacing,
                                               const DataType type) final;

    BrickedVolumePtr createBrickedVolume(const Vector3ui& dimensions, const Vector3f& spacing,
                                         const DataType type) final;

protected:
    void _commitTransferFunctionImpl(const Vector3fs& colors, const floats& opacities, const Vector2d valueRange) final;
    void _commitSimulationDataImpl(const float* frameData, const size_t frameSize) final;
};
} // namespace core

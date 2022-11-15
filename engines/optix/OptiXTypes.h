/* Copyright (c) 2015-2022, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
 *
 * This file is part of Brayns <https://github.com/BlueBrain/BioExplorer>
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

#include <glad/glad.h> // Needs to be included before gl_interop

#include "CommonStructs.h"

#include <optix.h>

#include <sutil/vec_math.h>

#include <memory>
#include <vector>

namespace brayns
{
typedef Record<RayGenData> RayGenRecord;
typedef Record<MissData> MissRecord;
typedef Record<HitGroupData> HitGroupRecord;

class OptiXCamera;
using OptiXCameraPtr = std::shared_ptr<OptiXCamera>;

const uint32_t maxTraceDepth = 12;
const uint32_t maxCCDepth = 0;
const uint32_t maxDCDepth = 0;
const uint32_t maxTraversableDepth = 1;
const uint32_t numPayloadValues = 5;
const uint32_t numAttributeValues = 5;

const BasicLight g_light = {
    make_float3(0.0f, 10.0f, 10.0f), // pos
    make_float3(1.0f, 1.0f, 1.0f)    // color
};

} // namespace brayns

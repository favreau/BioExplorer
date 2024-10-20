/*
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
 *
 * Copyright 2020-2023 Blue BrainProject / EPFL
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <optix_world.h>

#include <platform/engines/optix6/cuda/Context.cuh>

rtDeclareVariable(float, exponent, , );
rtDeclareVariable(uint, inverse, , );

static __device__ inline void shade()
{
    const float3 hit_point = ray.origin + t_hit * ray.direction;
    const float3 dir = ::optix::normalize(hit_point - eye);
    float3 world_shading_normal = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal));
    float cosNL = max(0.f, pow(::optix::dot(::optix::normalize(dir), -1.f * world_shading_normal), exponent));
    if (inverse)
        cosNL = 1.f - cosNL;

    prd.result = make_float4(cosNL, cosNL, cosNL, 1.f);
}

RT_PROGRAM void any_hit_shadow()
{
    rtTerminateRay();
}

RT_PROGRAM void closest_hit_radiance()
{
    shade();
}

RT_PROGRAM void closest_hit_radiance_textured()
{
    shade();
}

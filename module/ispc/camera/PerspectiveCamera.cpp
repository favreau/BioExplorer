/*
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * Copyright 2020-2021 Blue BrainProject / EPFL
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

#include "PerspectiveCamera.h"
#include <limits>

// ispc-side stuff
#include "PerspectiveCamera_ispc.h"

#include <ospray/SDK/common/Data.h>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <math.h> // M_PI
#endif

namespace bioexplorer
{
PerspectiveCamera::PerspectiveCamera()
{
    ispcEquivalent = ispc::PerspectiveCamera_create(this);
}

void PerspectiveCamera::commit()
{
    Camera::commit();

    // ------------------------------------------------------------------
    // first, "parse" the additional expected parameters
    // ------------------------------------------------------------------
    fovy = getParamf("fovy", 60.f);
    aspect = getParamf("aspect", 1.f);
    apertureRadius = getParamf("apertureRadius", 0.f);
    focusDistance = getParamf("focusDistance", 1.f);
    stereoMode = getParam("stereo", 0) ? CameraStereoMode::side_by_side
                                       : CameraStereoMode::mono;
    // the default 63.5mm represents the average human IPD
    interpupillaryDistance = getParamf("interpupillaryDistance", 0.0635f);
    enableClippingPlanes = getParam("enableClippingPlanes", 0);
    clipPlanes =
        enableClippingPlanes ? getParamData("clipPlanes", nullptr) : nullptr;

    // ------------------------------------------------------------------
    // now, update the local precomputed values
    // ------------------------------------------------------------------
    dir = normalize(dir);
    vec3f dir_du = normalize(cross(dir, up));
    vec3f dir_dv =
        cross(dir_du, dir); // rotate film to be perpendicular to 'dir'

    vec3f org = pos;
    const vec3f ipd_offset = 0.5f * interpupillaryDistance * dir_du;

    switch (stereoMode)
    {
    case CameraStereoMode::left:
        org -= ipd_offset;
        break;
    case CameraStereoMode::right:
        org += ipd_offset;
        break;
    case CameraStereoMode::side_by_side:
        aspect *= 0.5f;
        break;
    case CameraStereoMode::mono:
        break;
    }

    float imgPlane_size_y = 2.f * tanf(deg2rad(0.5f * fovy));
    float imgPlane_size_x = imgPlane_size_y * aspect;

    dir_du *= imgPlane_size_x;
    dir_dv *= imgPlane_size_y;

    vec3f dir_00 = dir - 0.5f * dir_du - 0.5f * dir_dv;

    float scaledAperture = 0.f;
    // prescale to focal plane
    if (apertureRadius > 0.f)
    {
        dir_du *= focusDistance;
        dir_dv *= focusDistance;
        dir_00 *= focusDistance;
        scaledAperture = apertureRadius / imgPlane_size_x;
    }

    const auto clipPlaneData = clipPlanes ? clipPlanes->data : nullptr;
    const size_t numClipPlanes = clipPlanes ? clipPlanes->numItems : 0;

    ispc::PerspectiveCamera_set(getIE(), (const ispc::vec3f&)org,
                                (const ispc::vec3f&)dir_00,
                                (const ispc::vec3f&)dir_du,
                                (const ispc::vec3f&)dir_dv, scaledAperture,
                                aspect, (const ispc::vec3f&)ipd_offset,
                                stereoMode, (const ispc::vec4f*)clipPlaneData,
                                numClipPlanes);
}

OSP_REGISTER_CAMERA(PerspectiveCamera, bio_explorer_perspective);

} // namespace bioexplorer

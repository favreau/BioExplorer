/* Copyright (c) 2015-2023, EPFL/Blue Brain Project
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

#include "OptiXContext.h"

#include <brayns/engineapi/Camera.h>
#include <optixu/optixpp_namespace.h>

namespace brayns
{

const std::string CUDA_FUNC_CAMERA_EXCEPTION = "exception";
const std::string CUDA_FUNC_CAMERA_ENVMAP_MISS = "envmap_miss";

const std::string CUDA_ATTR_CAMERA_BAD_COLOR = "bad_color";
const std::string CUDA_ATTR_CAMERA_OFFSET = "offset";
const std::string CUDA_ATTR_CAMERA_EYE = "eye";
const std::string CUDA_ATTR_CAMERA_U = "U";
const std::string CUDA_ATTR_CAMERA_V = "V";
const std::string CUDA_ATTR_CAMERA_W = "W";
const std::string CUDA_ATTR_CAMERA_ASPECT = "aspect";

/**
   OptiX specific camera

   This object is the OptiX specific implementation of a Camera
*/
class OptiXCamera : public Camera
{
public:
    /**
       Commits the changes held by the camera object so that
       attributes become available to the OptiX rendering engine
    */
    void commit() final;

private:
    optix::Buffer _clipPlanesBuffer{nullptr};
    Planes _clipPlanes;
    std::string _currentCamera;
};
} // namespace brayns
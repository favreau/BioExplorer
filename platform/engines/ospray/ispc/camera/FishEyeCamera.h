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

#pragma once

#include "camera/Camera.h"

#include <platform/core/common/CommonTypes.h>
#include <platform/core/common/Properties.h>
#include <platform/engines/ospray/OSPRayProperties.h>

namespace core
{
namespace engine
{
namespace ospray
{
struct OSPRAY_SDK_INTERFACE FishEyeCamera : public ::ospray::Camera
{
    FishEyeCamera();

    virtual std::string toString() const { return OSPRAY_CAMERA_PROPERTY_TYPE_FISHEYE; }
    virtual void commit();

public:
    // Clip planes
    bool enableClippingPlanes{false};
    ::ospray::Ref<::ospray::Data> clipPlanes;

    double aspect;
    float apertureRadius{DEFAULT_CAMERA_APERTURE_RADIUS};
    float focalDistance{DEFAULT_CAMERA_FOCAL_DISTANCE};
    float exposure{DEFAULT_COMMON_EXPOSURE};
    bool useHardwareRandomizer{DEFAULT_COMMON_USE_HARDWARE_RANDOMIZER};

    // Stereo
    CameraStereoMode stereoMode;
    double interpupillaryDistance;
};
} // namespace ospray
} // namespace engine
} // namespace core
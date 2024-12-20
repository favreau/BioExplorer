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

#include "OptiXContext.h"

#include <optixu/optixpp_namespace.h>
#include <platform/core/engineapi/Camera.h>

namespace core
{
namespace engine
{
namespace optix
{
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
    std::string _currentCamera;
};
} // namespace optix
} // namespace engine
} // namespace core
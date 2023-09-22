/*
 * Copyright (c) 2019, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
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

#include <optixu/optixpp_namespace.h>

#include "OptiXUtils.h"

namespace core
{
namespace engine
{
namespace optix
{
class OptiXCamera;

/**
 * @brief The OptiXCameraProgram class is an abstract class that provides the
 * required programs for launching rays from a camera
 */
class OptiXCameraProgram
{
public:
    virtual ~OptiXCameraProgram()
    {
        RT_DESTROY(_rayGenerationProgram);
        RT_DESTROY(_missProgram);
        RT_DESTROY(_exceptionProgram);
    }

    ::optix::Program getRayGenerationProgram() { return _rayGenerationProgram; }
    ::optix::Program getMissProgram() { return _missProgram; }
    ::optix::Program getExceptionProgram() { return _exceptionProgram; }
    /**
     * @brief commit Virtual method for committing camera specific variables to the context
     * @param camera The main core camera
     * @param context The OptiX context
     */
    virtual void commit(const OptiXCamera& camera, ::optix::Context context) = 0;

protected:
    ::optix::Program _rayGenerationProgram{nullptr};
    ::optix::Program _missProgram{nullptr};
    ::optix::Program _exceptionProgram{nullptr};
};
} // namespace optix
} // namespace engine
} // namespace core
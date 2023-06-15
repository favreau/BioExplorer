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

#include <platform/core/engineapi/FrameBuffer.h>

#include <mutex>

namespace core
{
using namespace solr;

class SolRFrameBuffer : public FrameBuffer
{
public:
    SolRFrameBuffer(const std::string& name, const Vector2ui& frameSize, FrameBufferFormat frameBufferFormat);
    ~SolRFrameBuffer();

    void clear() final;
    void resize(const Vector2ui& frameSize) final;
    void map() final;
    void unmap() final;
    void setAccumulation(const bool accumulation) final;

    std::unique_lock<std::mutex> getScopeLock() { return std::unique_lock<std::mutex>(_mapMutex); }
    const uint8_t* getColorBuffer() const final { return _colorBuffer; }
    const float* getFloatBuffer() const final { return _floatBuffer; }

private:
    void _recreate();
    void _unmapUnsafe();
    void _mapUnsafe();

    uint8_t* _colorBuffer;
    float* _floatBuffer;

    std::mutex _mapMutex;
};
} // namespace core

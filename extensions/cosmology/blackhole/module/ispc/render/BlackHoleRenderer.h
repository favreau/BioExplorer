/*
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
 *
 * Copyright 2020-2024 Blue BrainProject / EPFL
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

#pragma once

#include <platform/engines/ospray/ispc/render/utils/AbstractRenderer.h>

namespace spaceexplorer
{
namespace blackhole
{
class BlackHoleRenderer : public core::engine::ospray::AbstractRenderer
{
public:
    BlackHoleRenderer();

    /**
       Returns the class name as a string
       @return string containing the name of the object in the OSPRay context
    */
    std::string toString() const final { return "blackhole"; }
    void commit() final;

private:
    // Shading attributes
    float _exposure{1.f};
    ::ospray::uint32 _nbDisks;
    bool _grid{false};
    float _diskRotationSpeed{3.0};
    ::ospray::uint32 _diskTextureLayers{12};
    float _blackHoleSize{0.3f};
};
} // namespace blackhole
} // namespace spaceexplorer

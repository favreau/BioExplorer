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

#pragma once

#include "utils/SimulationRenderer.h"

namespace core
{
/**
 * @brief The AdvancedRenderer class is a renderer that can
 * perform global illumination (light shading, shadows, ambient occlusion, color
 * bleeding, light emission)
 */
class AdvancedRenderer : public SimulationRenderer
{
public:
    /**
     * @brief Construct a new Bio Explorer Renderer object
     *
     */
    AdvancedRenderer();

    /**
     * @brief Returns the class name as a string
     *
     * @return A string containing the full name of the class
     */
    std::string toString() const final { return "advanced_renderer"; }

    /**
     * @brief Commit the changes to the OSPRay engine
     *
     */
    void commit() final;

private:
    // Shading
    double _shadows{0.f};
    double _softShadows{0.f};
    ospray::uint32 _softShadowsSamples{1};

    double _giStrength{0.f};
    double _giDistance{1e6};
    ospray::uint32 _giSamples{1};

    bool _matrixFilter{false};

    // Clip planes
    ospray::Ref<ospray::Data> clipPlanes;
};
} // namespace core
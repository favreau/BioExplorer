/*
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
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

// Brayns
#include <brayns/ispc/render/utils/AdvancedMaterial.h>

// OSPRay
#include <ospray/SDK/render/Renderer.h>

namespace bioexplorer
{
namespace rendering
{
using namespace ospray;
using namespace brayns;

/**
 * @brief The FieldsRenderer class allows visualization of magnetic
 * fields created by atoms in the 3D scene. An Octree acceleration structure has
 * to be built by the be_build_fields API in order to feed the renderer with the
 * information needed to compute the value of the field for every point in the
 * 3D space
 */
class FieldsRenderer : public ospray::Renderer
{
public:
    /**
     * @brief Construct a new Bio Explorer Fields Renderer object
     *
     */
    FieldsRenderer();

    /**
     * @brief Returns the class name as a string
     *
     * @return A string containing the full name of the class
     */
    std::string toString() const final { return "bio_explorer_fields"; }

    /**
     * @brief Commit the changes to the OSPRay engine
     *
     */
    void commit() final;

private:
    // Shading attributes
    std::vector<void*> _lightArray;
    void** _lightPtr;
    ospray::Data* _lightData;

    AdvancedMaterial* _bgMaterial;

    bool _useHardwareRandomizer{false};
    ospray::uint32 _randomNumber{0};

    double _timestamp{0.f};
    double _exposure{1.f};

    double _alphaCorrection{1.f};

    // Octree
    double _minRayStep;
    ospray::uint32 _nbRaySteps;
    ospray::uint32 _nbRayRefinementSteps;

    double _cutoff;
    ospray::Ref<ospray::Data> _userData;
    ospray::uint64 _userDataSize;
};
} // namespace rendering
} // namespace bioexplorer
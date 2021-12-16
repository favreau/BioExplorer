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

#include "PointShape.h"

#include <plugin/common/Logs.h>

namespace bioexplorer
{
namespace common
{
using namespace brayns;
using namespace details;

PointShape::PointShape(const Vector4fs& clippingPlanes)
    : Shape(clippingPlanes)
{
    _bounds.merge(Vector3f());
}

Transformation PointShape::getTransformation(
    const uint64_t occurence, const uint64_t nbOccurences,
    const RandomizationDetails& randDetails, const float offset) const
{
    const Vector3f pos{0.f, 0.f, 0.f};
    const Quaterniond rot{0, 0, 0, 1};
    Transformation transformation;
    transformation.setTranslation(pos);
    transformation.setRotation(rot);
    return transformation;
}

Transformation PointShape::getTransformation(
    const uint64_t occurence, const uint64_t nbOccurences,
    const RandomizationDetails& randDetails, const float offset,
    const float /*morphingStep*/) const
{
    return getTransformation(occurence, nbOccurences, randDetails, offset);
}

bool PointShape::isInside(const Vector3f& point) const
{
    PLUGIN_THROW("isInside is not implemented for Plane shapes");
}

} // namespace common
} // namespace bioexplorer

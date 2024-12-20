/*
    Copyright 2020 - 2024 Blue Brain Project / EPFL

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

#include "Shape.h"

namespace bioexplorer
{
namespace common
{
class RNAShape : public Shape
{
public:
    /**
     * @brief Construct a new RNAShape object
     *
     * @param clippingPlanes Clipping planes to apply to the shape
     * @param shapeType Type of shape (Trefoil knot, star, spring, etc)
     * @param nbElements Number of elements in the RNA sequence
     * @param shapeParams Size of the shape
     * @param valuesRange Range of values for t
     * @param curveParams Curve parameters based on t, and depending on the
     * shape type
     */
    RNAShape(const Vector4ds& clippingPlanes, const details::RNAShapeType& shapeType, const uint64_t nbElements,
             const core::Vector2f& shapeParams, const core::Vector2f& valuesRange, const core::Vector3d& curveParams);

    /** @copydoc Shape::getTransformation */
    core::Transformation getTransformation(
        const uint64_t occurrence, const uint64_t nbOccurrences,
        const details::MolecularSystemAnimationDetails& MolecularSystemAnimationDetails,
        const double offset) const final;

    /** @copydoc Shape::isInside */
    bool isInside(const core::Vector3d& point) const final;

private:
    void _getSegment(const double u, const double v, core::Vector3d& src, core::Vector3d& dst) const;
    core::Vector3d _trefoilKnot(double t) const;
    core::Vector3d _torus(double t) const;
    core::Vector3d _star(double t) const;
    core::Vector3d _spring(double t) const;
    core::Vector3d _heart(double u) const;
    core::Vector3d _thing(double t) const;
    core::Vector3d _moebius(double u, double v) const;

    details::RNAShapeType _shapeType;

    core::Vector3d _U;
    core::Vector3d _V;
    double _step;

    core::Vector2d _shapeParams;
    core::Vector2d _valuesRange;
    core::Vector3d _curveParams;
};
typedef std::shared_ptr<RNAShape> RNAShapePtr;

} // namespace common
} // namespace bioexplorer

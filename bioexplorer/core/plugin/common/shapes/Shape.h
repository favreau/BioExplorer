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

#include <plugin/common/Types.h>

#include <brayns/common/Transformation.h>

namespace bioexplorer
{
namespace common
{
using namespace details;
using namespace brayns;

/**
 * @brief The shape class allows the creation of 3D shapes generated by a number
 * of element instances. Shapes can be a sphere, a cube, based on a mesh, etc.
 * Elements are molecules loaded from PDB files.
 */
class Shape
{
public:
    /**
     * @brief Construct a new Shape object
     *
     * @param clippingPlanes Clipping planes to apply to the shape
     */
    Shape(const Vector4ds& clippingPlanes);

    /**
     * @brief Destroy the Shape object
     *
     */
    ~Shape();

    /**
     * @brief Get the Transformation for the specified instance of the element
     *
     * @param occurrence Occurence of the element
     * @param nbOccurrences Total number of occurences in the shape
     * @param MolecularSystemAnimationDetails Details on how to animate elements
     * of the shape
     * @param offset Location offset of the element on the shape itself
     * @return Transformation Transformation of the instance
     */
    virtual Transformation getTransformation(const uint64_t occurrence, const uint64_t nbOccurrences,
                                             const MolecularSystemAnimationDetails& MolecularSystemAnimationDetails,
                                             const double offset = 0.0) const = 0;

    /**
     * @brief Return true if the specified 3D location is inside of the shape,
     * false if it is outside
     *
     * @param point 3D location in space
     * @return true if the 3D location is inside the shape
     * @return false if the 3D location is outside of the shape
     */
    virtual bool isInside(const Vector3d& point) const = 0;

    /**
     * @brief Get the total surface of the shape (in nanometers)
     *
     * @return double Suface of the shape
     */
    double getSurface() const { return _surface; }

    /**
     * @brief Get the bounds of the shape
     *
     * @return Boxf Bounds of the shape
     */
    Boxf getBounds() const { return _bounds; }

protected:
    Boxf _bounds;
    double _surface;
    Vector4ds _clippingPlanes;
};
typedef std::shared_ptr<Shape> ShapePtr;

} // namespace common
} // namespace bioexplorer

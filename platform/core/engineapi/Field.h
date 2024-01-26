/*
 * Copyright (c) 2015-2023, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Daniel Nachbaur <daniel.nachbaur@epfl.ch>
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

#include <platform/core/common/Api.h>
#include <platform/core/common/CommonTypes.h>
#include <platform/core/common/PropertyObject.h>

SERIALIZATION_ACCESS(Field)

namespace core
{
/**
 * @brief A field is volume in which voxels are computed in real-time using a pre-loaded Octree structure of events
 * defined by a 3D position and a value (float or Vector3)
 */
class Field : public PropertyObject
{
public:
    /**
     * @brief Constructs a new Field object.
     * @param dimensions The dimensions of the volume
     * @param spacing The spacing between voxels
     * @param offset Location of the octree in the 3D scene
     * @param indices Indices of the Octree
     * @param values Values of the Octree
     * @param dataType The data type of the field (point or vector)
     */
    PLATFORM_API Field(const FieldParameters& parameters, const Vector3ui& dimensions, const Vector3f& spacing,
                       const Vector3f& offset, const uint32_ts& indices, const floats& values,
                       const OctreeDataType dataType)
        : _parameters(parameters)
        , _dimensions(dimensions)
        , _spacing(spacing)
        , _offset(offset)
        , _octreeIndices(indices)
        , _octreeValues(values)
        , _octreeDataType(dataType)
    {
    }

    /**
     * @brief Gets the bounding box of the field
     * @return The bounding box of the field as a Boxd object.
     */
    PLATFORM_API Boxd getBounds() const { return {_offset, _offset + Vector3f(_dimensions) * _spacing}; }

    /**
     * @brief Get the Dimensions object
     *
     * @return The dimensions of the field in the 3D scene
     */
    PLATFORM_API Vector3i getDimensions() const { return _dimensions; }

    /**
     * @brief Get the Element Spacing object
     *
     * @return The voxel size
     */
    PLATFORM_API Vector3f getElementSpacing() const { return _spacing; }

    /**
     * @brief Get the Offset object
     *
     * @return The location of the field in the 3D scene
     */
    PLATFORM_API Vector3f getOffset() const { return _offset; }

    /**
     * @brief Get the Octree Indices object
     *
     * @return const uint32_ts& Indices in the octree
     */
    PLATFORM_API const uint32_ts& getOctreeIndices() const { return _octreeIndices; }

    /**
     * @brief Get the Octree Values object
     *
     * @return const floats& Values in the octree
     */
    PLATFORM_API const floats& getOctreeValues() const { return _octreeValues; }

    /**
     * @brief Get the Octree Data Type object
     *
     * @return OctreeDataType The data type
     */
    PLATFORM_API OctreeDataType getOctreeDataType() const { return _octreeDataType; }

protected:
    // Octree
    Vector3i _dimensions;
    Vector3f _spacing;
    Vector3f _offset;
    uint32_ts _octreeIndices;
    floats _octreeValues;
    OctreeDataType _octreeDataType;
    const FieldParameters& _parameters;

private:
    SERIALIZATION_FRIEND(Field);
};
} // namespace core
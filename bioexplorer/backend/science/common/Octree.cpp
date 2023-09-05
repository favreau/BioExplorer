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

#include "Octree.h"

#include <science/common/Logs.h>

using namespace core;

namespace bioexplorer
{
namespace common
{
typedef std::map<uint32_t, OctreeNode> OctreeLevelMap;

Octree::Octree(const floats &events, double voxelSize, const Vector3f &minAABB, const Vector3f &maxAABB)
    : _volumeDimensions(Vector3ui(0u, 0u, 0u))
    , _volumeSize(0u)
{
    PLUGIN_INFO(3, "Nb of events : " << events.size() / 5);

    // **************** Octree creations *******************
    // *****************************************************
    Vector3ui octreeSize(_pow2roundup(std::ceil((maxAABB.x - minAABB.x) / voxelSize)),
                         _pow2roundup(std::ceil((maxAABB.y - minAABB.y) / voxelSize)),
                         _pow2roundup(std::ceil((maxAABB.z - minAABB.z) / voxelSize)));

    // This octree is always cubic
    _octreeSize = std::max(std::max(octreeSize.x, octreeSize.y), octreeSize.z);

    PLUGIN_INFO(3, "Octree size  : " << _octreeSize);

    _depth = std::log2(_octreeSize) + 1u;
    std::vector<OctreeLevelMap> octree(_depth);

    PLUGIN_INFO(3, "Octree depth : " << _depth << " " << octree.size());

    if (_depth == 0)
        return;

    for (uint32_t i = 0; i < events.size(); i += 5)
    {
        PLUGIN_PROGRESS("Bulding octree from events", i, events.size());
        const uint32_t xpos = std::floor((events[i] - minAABB.x) / voxelSize);
        const uint32_t ypos = std::floor((events[i + 1] - minAABB.y) / voxelSize);
        const uint32_t zpos = std::floor((events[i + 2] - minAABB.z) / voxelSize);
        const double value = events[i + 4];

        const uint32_t indexX = xpos;
        const uint32_t indexY = ypos * (uint32_t)_octreeSize;
        const uint32_t indexZ = zpos * (uint32_t)_octreeSize * (uint32_t)_octreeSize;

        auto it = octree[0].find(indexX + indexY + indexZ);
        if (it == octree[0].end())
        {
            OctreeNode *child = nullptr;
            for (uint32_t level = 0; level < _depth; ++level)
            {
                bool newNode = false;
                const uint32_t divisor = std::pow(2, level);
                const Vector3f center(xpos, ypos, zpos);

                const uint32_t nBlock = _octreeSize / divisor;
                const uint32_t index = std::floor(xpos / divisor) + nBlock * std::floor(ypos / divisor) +
                                       nBlock * nBlock * std::floor(zpos / divisor);

                const double size = voxelSize * (level + 1u);

                if (octree[level].find(index) == octree[level].end())
                {
                    octree[level].insert(OctreeLevelMap::value_type(index, OctreeNode(center, size)));
                    newNode = true;
                }

                octree[level].at(index).addValue(value);

                if ((level > 0) && (child != nullptr))
                    octree[level].at(index).setChild(child);

                if (newNode)
                    child = &(octree[level].at(index));
                else
                    child = nullptr;
            }
        }
        else
        {
            for (uint32_t level = 0; level < _depth; ++level)
            {
                const uint32_t divisor = std::pow(2, level);
                const uint32_t nBlock = _octreeSize / divisor;
                const uint32_t index = std::floor(xpos / divisor) + nBlock * std::floor(ypos / divisor) +
                                       nBlock * nBlock * std::floor(zpos / divisor);
                octree[level].at(index).addValue(value);
            }
        }
    }
    for (uint32_t i = 0; i < octree.size(); ++i)
        PLUGIN_DEBUG("Number of leaves [" << i << "]: " << octree[i].size());

    // **************** Octree flattening *******************
    // ******************************************************

    _offsetPerLevel.resize(_depth);
    _offsetPerLevel[_depth - 1u] = 0;
    uint32_t previousOffset = 0u;
    for (uint32_t i = _depth - 1u; i > 0u; --i)
    {
        _offsetPerLevel[i - 1u] = previousOffset + octree[i].size();
        previousOffset = _offsetPerLevel[i - 1u];
    }

    uint32_t totalNodeNumber = 0;

    for (uint32_t i = 0; i < octree.size(); ++i)
        totalNodeNumber += octree[i].size();

    // need to be initialized with zeros
    _flatIndices.resize(totalNodeNumber * 2u, 0);
    _flatData.resize(totalNodeNumber * 4);

    // The root node
    _flattenChildren(&(octree[_depth - 1u].at(0)), _depth - 1u);

    // **************** Octree flattening end *****************
    // ********************************************************

    _volumeDimensions =
        Vector3ui(std::ceil((maxAABB.x - minAABB.x) / voxelSize), std::ceil((maxAABB.y - minAABB.y) / voxelSize),
                  std::ceil((maxAABB.z - minAABB.z) / voxelSize));
    _volumeSize = (uint32_t)_volumeDimensions.x * (uint32_t)_volumeDimensions.y * (uint32_t)_volumeDimensions.z;
}

Octree::~Octree() {}

void Octree::_flattenChildren(OctreeNode *node, uint32_t level)
{
    const std::vector<OctreeNode *> children = node->getChildren();

    if ((children.empty()) || (level == 0))
    {
        _flatData[_offsetPerLevel[level] * 4u] = node->getCenter().x;
        _flatData[_offsetPerLevel[level] * 4u + 1] = node->getCenter().y;
        _flatData[_offsetPerLevel[level] * 4u + 2] = node->getCenter().z;
        _flatData[_offsetPerLevel[level] * 4u + 3] = node->getValue();

        _offsetPerLevel[level] += 1u;
        return;
    }
    _flatData[_offsetPerLevel[level] * 4u] = node->getCenter().x;
    _flatData[_offsetPerLevel[level] * 4u + 1] = node->getCenter().y;
    _flatData[_offsetPerLevel[level] * 4u + 2] = node->getCenter().z;
    _flatData[_offsetPerLevel[level] * 4u + 3] = node->getValue();

    _flatIndices[_offsetPerLevel[level] * 2u] = _offsetPerLevel[level - 1];
    _flatIndices[_offsetPerLevel[level] * 2u + 1] = _offsetPerLevel[level - 1] + children.size() - 1u;
    _offsetPerLevel[level] += 1u;

    for (OctreeNode *child : children)
        _flattenChildren(child, level - 1u);
}
} // namespace common
} // namespace bioexplorer
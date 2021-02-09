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

#pragma once

#include <brayns/common/types.h>
#include <map>

namespace bioexplorer
{
using namespace brayns;

typedef std::map<size_t, Vector4fs> PointCloud;

class PointCloudMesher
{
public:
    /**
     * @brief Construct a new Point Cloud Mesher object
     *
     */
    PointCloudMesher();

    /**
     * @brief
     *
     * @param model
     * @param pointCloud
     * @return true
     * @return false
     */
    bool toConvexHull(Model& model, const PointCloud& pointCloud);

    /**
     * @brief
     *
     * @param model
     * @param pointCloud
     * @param gridSize
     * @param threshold
     * @return true
     * @return false
     */
    bool toMetaballs(brayns::Model& model, const PointCloud& pointCloud,
                     const size_t gridSize, const float threshold);
};

} // namespace bioexplorer

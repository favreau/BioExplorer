/*
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * Copyright 2020-2023 Blue Brain Project / EPFL
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

namespace sonataexplorer
{
namespace meshing
{
using namespace brayns;

typedef std::map<size_t, std::vector<Vector4f>> PointCloud;

class PointCloudMesher
{
public:
    PointCloudMesher();

    bool toConvexHull(Model& model, const PointCloud& pointCloud);

    bool toMetaballs(Model& model, const PointCloud& pointCloud,
                     const size_t gridSize, const float threshold);
};
} // namespace meshing
} // namespace sonataexplorer
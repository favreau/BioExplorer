/*
 * Copyright (c) 2015-2023, EPFL/Blue Brain Project
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
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

#include "TransferFunction.h"

#include <platform/core/common/Logs.h>

#include <algorithm>

namespace
{
double _interpolatedOpacity(const core::Vector2ds& controlPointsSorted, const double x)
{
    const auto& firstPoint = controlPointsSorted.front();
    if (x <= firstPoint.x)
        return firstPoint.y;

    for (size_t i = 1; i < controlPointsSorted.size(); ++i)
    {
        const auto& current = controlPointsSorted[i];
        const auto& previous = controlPointsSorted[i - 1];
        if (x <= current.x)
        {
            const auto t = (x - previous.x) / (current.x - previous.x);
            return (1.0 - t) * previous.y + t * current.y;
        }
    }

    const auto& lastPoint = controlPointsSorted.back();
    return lastPoint.y;
}
} // namespace

namespace core
{
bool ColorMap::operator==(const ColorMap& rhs) const
{
    if (this == &rhs)
        return true;
    return name == rhs.name && colors == rhs.colors;
}

void ColorMap::clear()
{
    colors = {{0, 0, 0}, {1, 1, 1}};
}

TransferFunction::TransferFunction()
{
    clear();
}

void TransferFunction::clear()
{
    _colorMap.clear();
    _controlPoints = {{0, 0}, {1, 1}};
    _valuesRange = {0, 255};
    markModified();
}

floats TransferFunction::calculateInterpolatedOpacities() const
{
    constexpr size_t numSamples = 256;
    constexpr double dx = 1. / (numSamples - 1);

    auto tfPoints = getControlPoints();
    std::sort(tfPoints.begin(), tfPoints.end(), [](auto a, auto b) { return a.x < b.x; });

    floats opacities;
    opacities.reserve(numSamples);
    for (size_t i = 0; i < numSamples; ++i)
        opacities.push_back(_interpolatedOpacity(tfPoints, i * dx));
    return opacities;
}
} // namespace core
/* Copyright (c) 2020-2022, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
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

#include "Vasculature.h"

#include <plugin/common/CommonTypes.h>
#include <plugin/common/Logs.h>
#include <plugin/common/Utils.h>

#include <plugin/io/db/DBConnector.h>

#include <brayns/common/Timer.h>
#include <brayns/engineapi/Material.h>
#include <brayns/engineapi/Model.h>
#include <brayns/engineapi/Scene.h>
#include <brayns/parameters/ParametersManager.h>

namespace bioexplorer
{
namespace vasculature
{
using namespace common;
using namespace io;
using namespace db;

Vasculature::Vasculature(Scene& scene, const VasculatureDetails& details)
    : SDFGeometries(details.radiusMultiplier == 0 ? 1.0
                                                  : details.radiusMultiplier,
                    doublesToVector3d(details.scale))
    , _details(details)
    , _scene(scene)
{
    Timer chrono;
    _buildModel();
    PLUGIN_TIMER(chrono.elapsed(), "Vasculature loaded");
}

void Vasculature::_addGraphSection(ThreadSafeContainer& container,
                                   const GeometryNode& srcNode,
                                   const GeometryNode& dstNode,
                                   const size_t materialId)
{
    const auto& src = srcNode.position;
    const auto& dst = dstNode.position;
    const auto userData = 0;
    const auto direction = dst - src;
    const auto maxRadius = std::max(srcNode.radius, dstNode.radius);
    const float radius = std::min(length(direction) / 5.0,
                                  maxRadius * _details.radiusMultiplier);
    container.addSphere(src, radius * 0.2, materialId, userData);
    container.addCone(src, radius * 0.2, Vector3f(src + direction * 0.79),
                      radius * 0.2, materialId, userData);
    container.addCone(dst, 0.0, Vector3f(src + direction * 0.8), radius,
                      materialId, userData);
    container.addCone(Vector3f(src + direction * 0.8), radius,
                      Vector3f(src + direction * 0.79), radius * 0.2,
                      materialId, userData);
}

void Vasculature::_addSimpleSection(ThreadSafeContainer& container,
                                    const GeometryNode& srcNode,
                                    const GeometryNode& dstNode,
                                    const size_t materialId)
{
    const auto& srcPoint = srcNode.position;
    const auto srcRadius = srcNode.radius * _details.radiusMultiplier;

    const auto& dstPoint = dstNode.position;
    const auto dstRadius = dstNode.radius * _details.radiusMultiplier;
    const auto userData = 0;

    if (!_details.useSdf)
    {
        container.addSphere(srcPoint, srcRadius, materialId, userData);
        container.addSphere(dstPoint, dstRadius, materialId, userData);
    }

    container.addCone(srcPoint, srcRadius, dstPoint, dstRadius, materialId,
                      userData, {},
                      Vector3f(segmentDisplacementStrength,
                               segmentDisplacementFrequency, 0.f));
}

void Vasculature::_addDetailedSection(ThreadSafeContainer& container,
                                      const GeometryNodes& nodes,
                                      const size_t baseMaterialId,
                                      const doubles& radii,
                                      const Vector2d& radiusRange)
{
    uint64_t geometryIndex = 0;
    Neighbours neighbours{};

    uint64_t i = 0;
    GeometryNode dstNode;
    for (const auto& node : nodes)
    {
        const auto& srcNode = node.second;
        const auto userData = node.first;

        size_t materialId = baseMaterialId;
        switch (_details.colorScheme)
        {
        case VasculatureColorScheme::radius:
            materialId = 256 * ((srcNode.radius - radiusRange.x) /
                                (radiusRange.y - radiusRange.x));
            break;
        case VasculatureColorScheme::section_points:
            materialId = 256 * double(node.first - nodes.begin()->first) /
                         double(nodes.size());
            break;
        }

        const auto& srcPoint = srcNode.position;
        const auto srcRadius =
            (userData < radii.size() ? radii[userData] : srcNode.radius) *
            _details.radiusMultiplier;
        const auto sectionId = srcNode.sectionId;

        if (i == 0 && !_details.useSdf)
            container.addSphere(srcPoint, srcRadius, materialId, userData);

        if (i > 0)
        {
            const auto dstPoint = dstNode.position;
            const double dstRadius =
                (userData < radii.size() ? radii[userData] : dstNode.radius) *
                _details.radiusMultiplier;

            if (!_details.useSdf)
                container.addSphere(dstPoint, dstRadius, materialId, userData);

            geometryIndex =
                container.addCone(dstPoint, dstRadius, srcPoint, srcRadius,
                                  materialId, userData, neighbours,
                                  Vector3f(segmentDisplacementStrength,
                                           segmentDisplacementFrequency, 0.f));
            neighbours = {geometryIndex};
        }

        dstNode = srcNode;
        ++i;
    }
}

void Vasculature::_buildModel(const doubles& radii)
{
    const auto useSdf =
        _details.quality == VasculatureQuality::low ? false : _details.useSdf;

    if (_modelDescriptor)
        _scene.removeModel(_modelDescriptor->getModelID());

    auto model = _scene.createModel();
    ThreadSafeContainers containers;

    PLUGIN_INFO(1, "Loading sections...");
    const auto sectionIds = DBConnector::getInstance().getVasculatureSections(
        _details.populationName, _details.sqlFilter);
    _nbSections = sectionIds.size();

    Vector2d radiusRange;
    if (_details.colorScheme == VasculatureColorScheme::radius)
        radiusRange = DBConnector::getInstance().getVasculatureRadiusRange(
            _details.populationName, _details.sqlFilter);

    uint64_t progress = 0;
    uint64_t index;
#pragma omp parallel for
    for (index = 0; index < _nbSections; ++index)
    {
        const auto sectionId = sectionIds[index];

        const auto filter = "section_guid=" + std::to_string(sectionId);
        const auto nodes = DBConnector::getInstance().getVasculatureNodes(
            _details.populationName, filter);

        ThreadSafeContainer container(*model, useSdf,
                                      doublesToVector3d(_details.scale));

        const auto& srcNode = nodes.begin()->second;
        auto iter = nodes.end();
        --iter;
        const auto& dstNode = iter->second;

        size_t materialId = 0;
        switch (_details.colorScheme)
        {
        case VasculatureColorScheme::section:
            materialId = dstNode.sectionId;
            break;
        case VasculatureColorScheme::subgraph:
            materialId = dstNode.graphId;
            break;
        case VasculatureColorScheme::pair:
            materialId = dstNode.pairId;
            break;
        case VasculatureColorScheme::entry_node:
            materialId = dstNode.entryNodeId;
            break;
        case VasculatureColorScheme::radius:
            materialId = 256 * ((srcNode.radius - radiusRange.x) /
                                (radiusRange.y - radiusRange.x));
            break;
        }

        switch (_details.quality)
        {
        case VasculatureQuality::low:
            _addGraphSection(container, srcNode, dstNode, materialId);
            break;
        case VasculatureQuality::medium:
            _addSimpleSection(container, srcNode, dstNode, materialId);
            break;
        default:
            _addDetailedSection(container, nodes, materialId, radii,
                                radiusRange);
            break;
        }
#pragma omp critical
        containers.push_back(container);

#pragma omp critical
        ++progress;

#pragma omp critical
        _nbNodes += nodes.size();

#pragma omp critical
        PLUGIN_PROGRESS("Loading " << _nbSections << " sections", progress,
                        _nbSections);
    }

    for (size_t i = 0; i < containers.size(); ++i)
    {
        PLUGIN_PROGRESS("- Compiling 3D geometry...", 1 + i, containers.size());
        auto& container = containers[i];
        container.commitToModel();
    }
    PLUGIN_INFO(1, "");

    const ModelMetadata metadata = {{"Number of nodes",
                                     std::to_string(_nbNodes)},
                                    {"Number of sections",
                                     std::to_string(_nbSections)}};

    _modelDescriptor.reset(new brayns::ModelDescriptor(std::move(model),
                                                       _details.assemblyName,
                                                       metadata));
    if (_modelDescriptor)
        _scene.addModel(_modelDescriptor);
    else
        PLUGIN_THROW("Vasculature model could not be created for population " +
                     _details.populationName);
}

void Vasculature::setRadiusReport(const VasculatureRadiusReportDetails& details)
{
    auto& connector = DBConnector::getInstance();
    const auto simulationReport =
        connector.getVasculatureSimulationReport(details.populationName,
                                                 details.simulationReportId);

    const size_t nbFrames =
        (simulationReport.endTime - simulationReport.startTime) /
        simulationReport.timeStep;
    if (nbFrames == 0)
        PLUGIN_THROW("Report does not contain any simulation data: " +
                     simulationReport.description);

    if (details.frame >= nbFrames)
        PLUGIN_THROW("Invalid frame specified for report: " +
                     simulationReport.description);
    const floats radii =
        connector.getVasculatureSimulationTimeSeries(details.populationName,
                                                     details.simulationReportId,
                                                     details.frame);
    doubles series;
    for (const double radius : radii)
        series.push_back(details.amplitude * radius);
    _buildModel(series);
}

} // namespace vasculature
} // namespace bioexplorer

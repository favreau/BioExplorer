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

#include "Vasculature.h"

#include <science/common/Logs.h>
#include <science/common/Utils.h>

#include <science/io/db/DBConnector.h>

#include <platform/core/common/Timer.h>
#include <platform/core/engineapi/Material.h>
#include <platform/core/engineapi/Model.h>
#include <platform/core/engineapi/Scene.h>
#include <platform/core/parameters/ParametersManager.h>

#include <omp.h>

using namespace core;

namespace bioexplorer
{
namespace vasculature
{
using namespace details;
using namespace common;
using namespace io;
using namespace db;

Vasculature::Vasculature(Scene& scene, const VasculatureDetails& details, const Vector3d& assemblyPosition,
                         const Quaterniond& assemblyRotation, const LoaderProgress& callback)
    : SDFGeometries(details.alignToGrid, assemblyPosition, assemblyRotation, doublesToVector3d(details.scale))
    , _details(details)
    , _scene(scene)
{
    _animationDetails = doublesToCellAnimationDetails(_details.animationParams);
    _spheresRepresentation.enabled = _details.representation == VasculatureRepresentation::spheres ||
                                     _details.representation == VasculatureRepresentation::uniform_spheres;
    _spheresRepresentation.uniform = _details.representation == VasculatureRepresentation::uniform_spheres;
    _spheresRepresentation.radius = _spheresRepresentation.uniform ? _details.radiusMultiplier : 0.f;

    Timer chrono;
    _buildModel(callback);
    PLUGIN_TIMER(chrono.elapsed(), "Vasculature loaded");
}

double Vasculature::_getDisplacementValue(const DisplacementElement& element)
{
    const auto params = _details.displacementParams;
    switch (element)
    {
    case DisplacementElement::vasculature_segment_strength:
        return valueFromDoubles(params, 0, DEFAULT_VASCULATURE_SEGMENT_STRENGTH);
    case DisplacementElement::vasculature_segment_frequency:
        return valueFromDoubles(params, 1, DEFAULT_VASCULATURE_SEGMENT_FREQUENCY);
    default:
        PLUGIN_THROW("Invalid displacement element");
    }
}

void Vasculature::_logRealismParams()
{
    PLUGIN_INFO(1, "----------------------------------------------------");
    PLUGIN_INFO(1, "Realism level (" << static_cast<uint32_t>(_details.realismLevel) << ")");
    PLUGIN_INFO(1,
                "- Section     : " << boolAsString(andCheck(static_cast<uint32_t>(_details.realismLevel),
                                                            static_cast<uint32_t>(VasculatureRealismLevel::section))));
    PLUGIN_INFO(1, "- Bifurcation : " << boolAsString(
                       andCheck(static_cast<uint32_t>(_details.realismLevel),
                                static_cast<uint32_t>(VasculatureRealismLevel::bifurcation))));
    PLUGIN_INFO(1, "----------------------------------------------------");
}

void Vasculature::_addGraphSection(ThreadSafeContainer& container, const GeometryNode& srcNode,
                                   const GeometryNode& dstNode, const size_t materialId)
{
    const auto userData = NO_USER_DATA;
    const auto useSdf = false;
    const auto maxRadius = std::max(srcNode.radius, dstNode.radius);
    const auto src = _animatedPosition(Vector4d(srcNode.position, maxRadius));
    const auto dst = _animatedPosition(Vector4d(dstNode.position, maxRadius));
    const auto direction = dst - src;

    const float radius = std::min(length(direction) / 5.0, _getCorrectedRadius(maxRadius, _details.radiusMultiplier));

    container.addSphere(src, radius * 0.2, materialId, useSdf, userData);
    container.addCone(src, radius * 0.2, Vector3f(src + direction * 0.79), radius * 0.2, materialId, useSdf, userData);
    container.addCone(dst, 0.0, Vector3f(src + direction * 0.8), radius, materialId, useSdf, userData);
    container.addCone(Vector3f(src + direction * 0.8), radius, Vector3f(src + direction * 0.79), radius * 0.2,
                      materialId, useSdf, userData);
}

void Vasculature::_addSimpleSection(ThreadSafeContainer& container, const GeometryNode& srcNode,
                                    const GeometryNode& dstNode, const size_t materialId, const uint64_t userData)
{
    const auto srcRadius = _getCorrectedRadius(srcNode.radius, _details.radiusMultiplier);
    const auto& srcPoint = _animatedPosition(Vector4d(srcNode.position, srcRadius));

    const auto dstRadius = _getCorrectedRadius(dstNode.radius, _details.radiusMultiplier);
    const auto& dstPoint = _animatedPosition(Vector4d(dstNode.position, dstRadius));

    const auto useSdf =
        andCheck(static_cast<uint32_t>(_details.realismLevel), static_cast<uint32_t>(VasculatureRealismLevel::section));
    if (_spheresRepresentation.enabled)
        container.addConeOfSpheres(srcPoint, srcRadius, dstPoint, dstRadius, materialId, userData,
                                   _spheresRepresentation.radius);
    else
    {
        if (!useSdf)
        {
            container.addSphere(srcPoint, srcRadius, materialId, useSdf, userData);
            container.addSphere(dstPoint, dstRadius, materialId, useSdf, userData);
        }

        container.addCone(srcPoint, srcRadius, dstPoint, dstRadius, materialId, useSdf, userData, {},
                          Vector3f(_getDisplacementValue(DisplacementElement::vasculature_segment_strength),
                                   _getDisplacementValue(DisplacementElement::vasculature_segment_frequency), 0.f));
    }
}

void Vasculature::_addDetailedSection(ThreadSafeContainer& container, const GeometryNodes& nodes,
                                      const size_t baseMaterialId, const doubles& radii, const Vector2d& radiusRange)
{
    uint64_t geometryIndex = 0;
    Neighbours neighbours;

    const auto useSdf =
        andCheck(static_cast<uint32_t>(_details.realismLevel), static_cast<uint32_t>(VasculatureRealismLevel::section));

    GeometryNodes localNodes;
    switch (_details.representation)
    {
    case VasculatureRepresentation::optimized_segment:
    {
        double oldRadius = 0.0;
        double segmentLength = 0.0;
        for (const auto& node : nodes)
        {
            segmentLength += node.second.radius;
            if (segmentLength < 2.0 * (oldRadius + node.second.radius))
            {
                GeometryNode n;
                n.position = node.second.position;
                n.radius = node.second.radius;
                localNodes[node.first] = n;
            }
            else
                segmentLength = 0.0;
            oldRadius = node.second.radius;
        }
        break;
    }
    case VasculatureRepresentation::bezier:
    {
        Vector4fs points;
        uint64_ts ids;
        for (const auto& node : nodes)
        {
            points.push_back(
                Vector4d(node.second.position.x, node.second.position.y, node.second.position.z, node.second.radius));
            ids.push_back(node.first);
        }
        const auto localPoints = _getProcessedSectionPoints(morphology::MorphologyRepresentation::bezier, points);

        uint64_t i = 0;
        for (const auto& point : localPoints)
        {
            GeometryNode n;
            n.position = Vector3d(point.x, point.y, point.y);
            n.radius = point.w;
            localNodes[ids[i * (nodes.size() / localPoints.size())]] = n;
            ++i;
        }
    }
    default:
        localNodes = nodes;
    }

    uint64_t i = 0;
    GeometryNode dstNode;
    for (const auto& node : localNodes)
    {
        const auto& srcNode = node.second;
        const auto userData = node.first;

        size_t materialId;
        switch (_details.colorScheme)
        {
        case VasculatureColorScheme::radius:
            materialId = 256 * ((srcNode.radius - radiusRange.x) / (radiusRange.y - radiusRange.x));
            break;
        case VasculatureColorScheme::section_points:
            materialId = 256 * double(node.first - nodes.begin()->first) / double(nodes.size());
            break;
        default:
            materialId = baseMaterialId;
            break;
        }

        const float srcRadius = _getCorrectedRadius((userData < radii.size() ? radii[userData] : srcNode.radius),
                                                    _details.radiusMultiplier);
        const auto srcPosition = _animatedPosition(Vector4d(srcNode.position, srcRadius));

        if (i == 0)
        {
            if (!_spheresRepresentation.enabled)
                container.addSphere(srcPosition, srcRadius, materialId, useSdf, userData);
        }
        else
        {
            const float dstRadius = _getCorrectedRadius((userData < radii.size() ? radii[userData] : dstNode.radius),
                                                        _details.radiusMultiplier);
            const auto dstPosition = _animatedPosition(Vector4d(dstNode.position, dstRadius));

            if (_spheresRepresentation.enabled)
                container.addConeOfSpheres(srcPosition, srcRadius, dstPosition, dstRadius, materialId, userData,
                                           _spheresRepresentation.radius);
            else
            {
                geometryIndex = container.addCone(
                    srcPosition, srcRadius, dstPosition, dstRadius, materialId, useSdf, userData, neighbours,
                    Vector3f(_getDisplacementValue(DisplacementElement::vasculature_segment_strength),
                             _getDisplacementValue(DisplacementElement::vasculature_segment_frequency), 0.f));
                neighbours = {geometryIndex};

                if (!useSdf)
                    neighbours.insert(container.addSphere(srcPosition, srcRadius, materialId, useSdf, userData));
            }
        }

        dstNode = srcNode;
        ++i;
    }
}

void Vasculature::_addOrientation(ThreadSafeContainer& container, const GeometryNodes& nodes, const uint64_t sectionId)
{
    const auto nbNodes = nodes.size();
    if (nbNodes <= 3)
        return;

    StreamlinesData streamline;

    GeometryNode previousNode;
    const float alpha = 1.f;
    uint64_t i = 0;
    for (const auto& node : nodes)
    {
        streamline.vertex.push_back(
            Vector4f(node.second.position, _getCorrectedRadius(node.second.radius, _details.radiusMultiplier)));
        streamline.vertexColor.push_back(
            (i == 0 ? Vector4f(0.f, 0.f, 0.f, alpha)
                    : Vector4f(0.5 + 0.5 * normalize(node.second.position - previousNode.position), alpha)));
        previousNode = node.second;
        ++i;
    }

    container.addStreamline(sectionId, streamline);
}

void Vasculature::_buildModel(const LoaderProgress& callback, const doubles& radii)
{
    if (_modelDescriptor)
        _scene.removeModel(_modelDescriptor->getModelID());

    auto model = _scene.createModel();
    ThreadSafeContainers containers;

    PLUGIN_INFO(1, "Identifying nodes...");
    callback.updateProgress("Identifying nodes...", 1.f);
    const auto nbDBConnections = DBConnector::getInstance().getNbConnections();

    _nbNodes = DBConnector::getInstance().getVasculatureNbNodes(_details.populationName, _details.sqlFilter);

    const auto dbBatchSize = _nbNodes / nbDBConnections;
    PLUGIN_INFO(1, "DB connections=" << nbDBConnections << ", DB batch size=" << dbBatchSize);

    Vector2d radiusRange;
    if (_details.colorScheme == VasculatureColorScheme::radius)
        radiusRange = DBConnector::getInstance().getVasculatureRadiusRange(_details.populationName, _details.sqlFilter);

    uint64_t index;
    volatile bool flag = false;
    std::string flagMessage;
#pragma omp parallel for shared(flag) num_threads(nbDBConnections)
    for (index = 0; index < nbDBConnections; ++index)
    {
        try
        {
            if (flag)
                continue;

            if (omp_get_thread_num() == 0)
            {
                PLUGIN_PROGRESS("Loading sections...", index, nbDBConnections);
                try
                {
                    callback.updateProgress("Loading sections...", 0.5f * ((float)index / (float)nbDBConnections));
                }
                catch (...)
                {
#pragma omp critical
                    {
                        flag = true;
                    }
                }
            }

            const auto offset = index * dbBatchSize;
            const std::string limits = "OFFSET " + std::to_string(offset) + " LIMIT " + std::to_string(dbBatchSize);

            const auto filter = _details.sqlFilter;
            const auto nodes = DBConnector::getInstance().getVasculatureNodes(_details.populationName, filter, limits);

            if (nodes.empty())
                continue;

            ThreadSafeContainer container(*model, _alignToGrid, _position, _rotation,
                                          doublesToVector3d(_details.scale));

            auto iter = nodes.begin();
            uint64_t previousSectionId = iter->second.sectionId;
            do
            {
                GeometryNodes sectionNodes;
                const auto sectionId = iter->second.sectionId;
                const auto userData = iter->first;
                while (iter != nodes.end() && iter->second.sectionId == previousSectionId)
                {
                    sectionNodes[iter->first] = iter->second;
                    ++iter;
                }
                previousSectionId = sectionId;

                if (sectionNodes.size() >= 1)
                {
                    const auto& srcNode = sectionNodes.begin()->second;
                    auto it = sectionNodes.end();
                    --it;
                    const auto& dstNode = it->second;

                    size_t materialId;
                    switch (_details.colorScheme)
                    {
                    case VasculatureColorScheme::section:
                        materialId = sectionId;
                        break;
                    case VasculatureColorScheme::section_orientation:
                        materialId = getMaterialIdFromOrientation(dstNode.position - srcNode.position);
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
                        materialId = 256 * ((srcNode.radius - radiusRange.x) / (radiusRange.y - radiusRange.x));
                        break;
                    case VasculatureColorScheme::region:
                        materialId = dstNode.regionId;
                        break;
                    default:
                        materialId = 0;
                        break;
                    }

                    switch (_details.representation)
                    {
                    case VasculatureRepresentation::graph:
                        _addGraphSection(container, srcNode, dstNode, materialId);
                        break;
                    case VasculatureRepresentation::section:
                        _addSimpleSection(container, srcNode, dstNode, materialId, userData);
                        break;
                    default:
                        _addDetailedSection(container, sectionNodes, materialId, radii, radiusRange);
                        break;
                    }
                }
            } while (iter != nodes.end());

#pragma omp critical
            {
                containers.push_back(container);
            }
        }
        catch (const std::runtime_error& e)
        {
#pragma omp critical
            {
                flagMessage = e.what();
                flag = true;
            }
        }
        catch (...)
        {
#pragma omp critical
            {
                flagMessage = "Loading was canceled";
                flag = true;
            }
        }
    }

    for (size_t i = 0; i < containers.size(); ++i)
    {
        PLUGIN_PROGRESS("- Compiling 3D geometry...", 1 + i, containers.size());
        callback.updateProgress("Compiling 3D geometry...", 0.5f + 0.5f * (float)(1 + i) / (float)containers.size());
        auto& container = containers[i];
        container.commitToModel();
    }
    model->applyDefaultColormap();

    const ModelMetadata metadata = {{"Number of nodes", std::to_string(_nbNodes)}, {"SQL filter", _details.sqlFilter}};

    _modelDescriptor.reset(new core::ModelDescriptor(std::move(model), _details.assemblyName, metadata));

    if (!_modelDescriptor)
        PLUGIN_THROW(
            "Vasculature model could not be created for "
            "population " +
            _details.populationName);
}

void Vasculature::setRadiusReport(const VasculatureRadiusReportDetails& details)
{
    auto& connector = DBConnector::getInstance();
    const auto simulationReport = connector.getSimulationReport(details.populationName, details.simulationReportId);

    const size_t nbFrames = (simulationReport.endTime - simulationReport.startTime) / simulationReport.timeStep;
    if (nbFrames == 0)
        PLUGIN_THROW("Report does not contain any simulation data: " + simulationReport.description);

    if (details.frame >= nbFrames)
        PLUGIN_THROW("Invalid frame specified for report: " + simulationReport.description);
    const floats radii =
        connector.getVasculatureSimulationTimeSeries(details.populationName, details.simulationReportId, details.frame);
    doubles series;
    for (const double radius : radii)
        series.push_back(details.amplitude * radius);
    _buildModel(LoaderProgress(), series);
}

} // namespace vasculature
} // namespace bioexplorer

/* Copyright (c) 2018-2021, EPFL/Blue Brain Project
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

#include "Astrocytes.h"

#include <plugin/common/CommonTypes.h>
#include <plugin/common/Logs.h>
#include <plugin/common/ParallelModelContainer.h>

#include <plugin/io/db/DBConnector.h>

#include <brayns/common/Timer.h>
#include <brayns/engineapi/Material.h>
#include <brayns/engineapi/Model.h>
#include <brayns/engineapi/Scene.h>

namespace bioexplorer
{
namespace morphology
{
using namespace common;
using namespace io;
using namespace db;

const double DEFAULT_SOMA_DISPLACEMENT = 2.0;
const double DEFAULT_SECTION_DISPLACEMENT = 2.0;
const double DEFAULT_MITOCHONDRIA_DENSITY = 0.0459;

Astrocytes::Astrocytes(Scene& scene, const AstrocytesDetails& details)
    : Morphologies(details.useSdf, details.radiusMultiplier, details.scale)
    , _details(details)
    , _scene(scene)
{
    Timer chrono;
    _buildModel();
    PLUGIN_TIMER(chrono.elapsed(), "Astrocytes loaded");
}

void Astrocytes::_buildModel(const doubles& radii)
{
    PLUGIN_ERROR("Building astrocytes");

    if (_modelDescriptor)
        _scene.removeModel(_modelDescriptor->getModelID());

    auto& connector = DBConnector::getInstance();

    auto model = _scene.createModel();
    std::set<uint64_t> materialIds;
    SDFMorphologyData sdfMorphologyData;
    const auto useSdf = _details.useSdf;
    const auto somas = connector.getAstrocytes(_details.sqlFilter);

    // Astrocytes
    size_t baseMaterialId = 0;
    const uint64_t userData = 0;

    std::vector<ParallelModelContainer> containers;
    uint64_t morphologyId;
#pragma omp parallel for private(morphologyId)
    for (morphologyId = 0; morphologyId < somas.size(); ++morphologyId)
    {
        auto it = somas.begin();
        std::advance(it, morphologyId);
        const auto& soma = it->second;
        const auto somaId = it->first;
        PLUGIN_PROGRESS("Loading astrocytes", morphologyId, somas.size());

        ParallelModelContainer modelContainer;
        const auto& somaPosition = soma.center;

        // Load data from DB
        double somaRadius = 0.0;
        SectionMap sections;
        if (_details.loadSomas || _details.loadDendrites)
            sections = connector.getAstrocyteSections(somaId);

        EndFootMap endFeet;
        if (_details.loadEndFeet)
            endFeet = connector.getAstrocyteEndFeet(somaId);

        // Soma radius
        uint64_t count = 1;
        for (const auto& section : sections)
            if (section.second.parentId == SOMA_AS_PARENT)
            {
                const auto& point = section.second.points[0];
                somaRadius += 0.75 * length(Vector3d(point));
                ++count;
            }
        somaRadius = _radiusMultiplier * somaRadius / count;

        // Color scheme
        switch (_details.populationColorScheme)
        {
        case PopulationColorScheme::id:
            baseMaterialId = somaId * NB_MATERIALS_PER_MORPHOLOGY;
            break;
        default:
            baseMaterialId = 0;
        }
        materialIds.insert(baseMaterialId);
        const auto somaMaterialId =
            baseMaterialId +
            (_details.morphologyColorScheme == MorphologyColorScheme::section
                 ? MATERIAL_OFFSET_SOMA
                 : 0);
        materialIds.insert(somaMaterialId);

        uint64_t somaGeometryIndex = 0;
        if (_details.loadSomas)
        {
            somaGeometryIndex =
                _addSphere(useSdf, somaPosition, somaRadius, somaMaterialId,
                           NO_USER_DATA, modelContainer, sdfMorphologyData, {},
                           DEFAULT_SOMA_DISPLACEMENT);
            if (_details.generateInternals)
            {
                _addSomaInternals(somaId, modelContainer, baseMaterialId,
                                  somaPosition, somaRadius,
                                  DEFAULT_MITOCHONDRIA_DENSITY,
                                  sdfMorphologyData);
                materialIds.insert(baseMaterialId + MATERIAL_OFFSET_NUCLEUS);
                materialIds.insert(baseMaterialId +
                                   MATERIAL_OFFSET_MITOCHONDRION);
            }
        }

        Neighbours neighbours;
        neighbours.insert(somaGeometryIndex);
        for (const auto& section : sections)
        {
            uint64_t geometryIndex = 0;
            const auto& points = section.second.points;

            size_t sectionMaterialId = baseMaterialId;
            const auto sectionId = section.first;
            switch (_details.morphologyColorScheme)
            {
            case MorphologyColorScheme::section:
                sectionMaterialId = baseMaterialId + section.second.type;
                break;
            default:
                break;
            }
            materialIds.insert(sectionMaterialId);

            size_t step = 1;
            switch (_details.geometryQuality)
            {
            case GeometryQuality::low:
                step = points.size() - 2;
                break;
            default:
                break;
            }

            if (_details.loadDendrites)
            {
                uint64_t geometryIndex = 0;
                if (section.second.parentId == SOMA_AS_PARENT)
                {
                    // Section connected to the soma
                    const auto& point = points[0];
                    geometryIndex =
                        _addCone(useSdf, somaPosition,
                                 somaRadius * 0.75 * _radiusMultiplier,
                                 somaPosition + Vector3d(point),
                                 point.w * 0.5 * _radiusMultiplier,
                                 somaMaterialId, userData, modelContainer,
                                 sdfMorphologyData, neighbours,
                                 DEFAULT_SOMA_DISPLACEMENT);
                    neighbours.insert(geometryIndex);
                }

                for (uint64_t i = 0; i < points.size() - 1; i += step)
                {
                    const auto srcPoint = points[i];
                    const auto src = somaPosition + Vector3d(srcPoint);
                    const float srcRadius =
                        srcPoint.w * 0.5 * _radiusMultiplier;

                    // Ignore points that are too close the previous one
                    // (according to respective radii)
                    Vector4f dstPoint;
                    float dstRadius;
                    do
                    {
                        dstPoint = points[i + step];
                        dstRadius = dstPoint.w * 0.5 * _radiusMultiplier;
                        ++i;
                    } while (length(Vector3f(dstPoint) - Vector3f(srcPoint)) <
                                 (srcRadius + dstRadius) &&
                             (i + step) < points.size() - 1);
                    --i;

                    const auto dst = somaPosition + Vector3d(dstPoint);
                    if (!useSdf)
                        geometryIndex =
                            _addSphere(useSdf, dst, dstRadius,
                                       sectionMaterialId, NO_USER_DATA,
                                       modelContainer, sdfMorphologyData, {});

                    geometryIndex =
                        _addCone(useSdf, src, srcRadius, dst, dstRadius,
                                 sectionMaterialId, userData, modelContainer,
                                 sdfMorphologyData, {geometryIndex},
                                 DEFAULT_SECTION_DISPLACEMENT);

                    _bounds.merge(srcPoint);
                }
            }
        }

        if (_details.loadEndFeet && !_details.vasculaturePopulationName.empty())
            _addEndFoot(endFeet, radii, somaMaterialId, sdfMorphologyData,
                        modelContainer);
#pragma omp critical
        containers.push_back(modelContainer);
    }

    for (size_t i = 0; i < containers.size(); ++i)
    {
        const float progress = 1.f + i;
        PLUGIN_PROGRESS("- Compiling 3D geometry...", progress,
                        containers.size());
        auto& container = containers[i];
        container.moveGeometryToModel(*model);
    }

    _createMaterials(materialIds, *model);

    if (useSdf)
        _finalizeSDFGeometries(*model, sdfMorphologyData);

    ModelMetadata metadata = {
        {"Number of astrocytes", std::to_string(somas.size())}};

    _modelDescriptor.reset(new brayns::ModelDescriptor(std::move(model),
                                                       _details.assemblyName,
                                                       metadata));
    if (_modelDescriptor)
        _scene.addModel(_modelDescriptor);
    else
        PLUGIN_THROW("Astrocytes model could not be created");
}

void Astrocytes::_addEndFoot(const EndFootMap& endFeet, const doubles& radii,
                             const size_t materialId,
                             SDFMorphologyData& sdfMorphologyData,
                             ParallelModelContainer& model)
{
    const double DEFAULT_ENDFOOT_RADIUS_RATIO = 1.2;
    const auto radiusMultiplier = _details.radiusMultiplier;
    const auto useSdf = _details.useSdf;
    for (const auto& endFoot : endFeet)
    {
        for (const auto& node : endFoot.second.nodes)
        {
            const auto& connector = DBConnector::getInstance();
            const auto vasculatureNodes = connector.getVasculatureNodes(
                _details.vasculaturePopulationName,
                "section_guid=" +
                    std::to_string(endFoot.second.vasculatureSectionId));

            uint64_t startIndex = 0;
            uint64_t endIndex = 1;
            const auto halfLength = endFoot.second.length / 2.0;
            auto it = vasculatureNodes.begin();
            std::advance(it, endFoot.second.vasculatureSegmentId);
            const auto centerPosition = it->second.position;

            double length = 0.0;
            int64_t i = -1;
            // Find start segment making the assumption that the segment Id is
            // in the middle of the end-foot
            while (length < halfLength &&
                   endFoot.second.vasculatureSegmentId + i >= 0)
            {
                const int64_t segmentId =
                    endFoot.second.vasculatureSegmentId + i;
                if (segmentId < 0)
                    break;
                auto it = vasculatureNodes.begin();
                std::advance(it, segmentId);
                length = glm::length(centerPosition - it->second.position);
                startIndex = segmentId;
                --i;
            }

            length = 0.0;
            i = 1;
            // Now find the end segment
            while (length < halfLength &&
                   endFoot.second.vasculatureSegmentId + i <
                       vasculatureNodes.size())
            {
                const int64_t segmentId =
                    endFoot.second.vasculatureSegmentId + i;
                auto it = vasculatureNodes.begin();
                std::advance(it, segmentId);
                length = glm::length(centerPosition - it->second.position);
                endIndex = segmentId;
                ++i;
            }

            // Build the segment using spheres
            for (uint64_t i = startIndex; i < endIndex - 1; ++i)
            {
                auto it = vasculatureNodes.begin();
                std::advance(it, i);
                const auto& startNode = it->second;
                const auto startRadius =
                    (it->first < radii.size() ? radii[it->first]
                                              : startNode.radius) *
                    DEFAULT_ENDFOOT_RADIUS_RATIO * radiusMultiplier;

                std::advance(it, 1);
                const auto& endNode = it->second;
                const auto endRadius =
                    (it->first < radii.size() ? radii[it->first]
                                              : startNode.radius) *
                    DEFAULT_ENDFOOT_RADIUS_RATIO * radiusMultiplier;

                if (!_details.useSdf)
                    _addSphere(useSdf, startNode.position, startRadius,
                               materialId, NO_USER_DATA, model,
                               sdfMorphologyData, {},
                               DEFAULT_SECTION_DISPLACEMENT);
                _addCone(useSdf, startNode.position, startRadius,
                         endNode.position, endRadius, materialId, NO_USER_DATA,
                         model, sdfMorphologyData, {},
                         DEFAULT_SECTION_DISPLACEMENT);
            }
        }
    }
}

void Astrocytes::setVasculatureRadiusReport(
    const VasculatureRadiusReportDetails& details)
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
        connector.getVasculatureSimulationTimeSeries(details.simulationReportId,
                                                     details.frame);
    doubles series;
    for (const double radius : radii)
        series.push_back(details.amplitude * radius);
    _buildModel(series);
}

} // namespace morphology
} // namespace bioexplorer

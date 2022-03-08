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

#include "Neurons.h"

#include <plugin/common/CommonTypes.h>
#include <plugin/common/Logs.h>
#include <plugin/common/Utils.h>

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

const size_t NB_MATERIALS_PER_NEURON = 10;
const size_t MATERIAL_OFFSET_SOMA = 0;
const size_t MATERIAL_OFFSET_AXON = 1;
const size_t MATERIAL_OFFSET_DENDRITE = 2;
const size_t MATERIAL_OFFSET_APICAL_DENDRITE = 3;
const size_t MATERIAL_OFFSET_AFFERENT_SYNPASE = 4;
const size_t MATERIAL_OFFSET_EFFERENT_SYNPASE = 5;
const size_t MATERIAL_OFFSET_MITOCHONDRION = 6;
const size_t MATERIAL_OFFSET_NUCLEUS = 7;
const size_t MATERIAL_OFFSET_MYELIN_SHEATH = 8;

const int64_t SOMA_AS_PARENT = -1;
const uint64_t NB_MYELIN_FREE_SEGMENTS = 4;

const double DEFAULT_SOMA_DISPLACEMENT = 2.0;
const double DEFAULT_SECTION_DISPLACEMENT = 10.0;

// Mitochondria density per layer
doubles MITOCHONDRIA_DENSITY = {0.0459, 0.0522, 0.064, 0.0774, 0.0575, 0.0403};

Neurons::Neurons(Scene& scene, const NeuronsDetails& details)
    : _details(details)
    , _scene(scene)
{
    Timer chrono;
    _buildNeuron();
    PLUGIN_TIMER(chrono.elapsed(), "Neurons loaded");
}

void Neurons::_buildNeuron()
{
    auto& connector = DBConnector::getInstance();

    auto model = _scene.createModel();
    MaterialSet materialIds;
    SDFMorphologyData sdfMorphologyData;
    const auto useSdf = _details.useSdf;
    const auto somas = connector.getNeurons(_details.sqlNodeFilter);

    // Neurons
    size_t previousMaterialId = std::numeric_limits<size_t>::max();
    size_t baseMaterialId = 0;
    Vector3ui indexOffset;

    for (const auto& soma : somas)
    {
        const auto somaId = soma.first;
        const auto& somaPosition = soma.second.position;
        const auto somaRadius = _details.radiusMultiplier;
        const auto layer = soma.second.layer;
        const double mitochondriaDensity =
            (layer < MITOCHONDRIA_DENSITY.size() ? MITOCHONDRIA_DENSITY[layer]
                                                 : 0.0);

        PLUGIN_PROGRESS("Loading Neurons", soma.first, somas.size());
        switch (_details.populationColorScheme)
        {
        case PopulationColorScheme::id:
            baseMaterialId = somaId * NB_MATERIALS_PER_NEURON;
            break;
        default:
            baseMaterialId = static_cast<size_t>(NeuronSectionType::soma);
        }
        materialIds.insert(baseMaterialId);

        // Soma
        uint64_t somaGeometryIndex = 0;
        if (_details.loadSomas)
        {
            somaGeometryIndex =
                _addSphere(useSdf, somaPosition, somaRadius, baseMaterialId,
                           NO_USER_DATA, *model, sdfMorphologyData, {},
                           DEFAULT_SOMA_DISPLACEMENT);
            if (_details.generateInternals)
            {
                _addSomaInternals(somaId, *model, baseMaterialId, somaPosition,
                                  somaRadius, mitochondriaDensity,
                                  sdfMorphologyData);
                materialIds.insert(baseMaterialId + MATERIAL_OFFSET_NUCLEUS);
                materialIds.insert(baseMaterialId +
                                   MATERIAL_OFFSET_MITOCHONDRION);
            }
        }

        // Sections (dendrites and axon)
        if (_details.loadBasalDendrites || _details.loadApicalDendrites ||
            _details.loadAxons)
        {
            const auto sections =
                connector.getNeuronSections(somaId, _details.sqlSectionFilter);

            uint64_t geometryIndex = 0;
            Neighbours neighbours;
            neighbours.insert(somaGeometryIndex);

            for (const auto& section : sections)
            {
                if (section.second.parentId == SOMA_AS_PARENT)
                {
                    const Vector4f somaPoint{somaPosition.x, somaPosition.y,
                                             somaPosition.z, somaRadius};
                    Vector4f averagePoint;
                    for (uint64_t i = 0; i < 1; ++i)
                    {
                        averagePoint += somaPoint;
                        averagePoint += somaPoint;
                        averagePoint += somaPoint + section.second.points[i];
                    }
                    averagePoint /= 3.f;

                    // Sphere at half way between soma and 2nd point in the
                    // section
                    const auto& point = section.second.points[1];
                    Vector3f position =
                        (somaPosition + somaPosition + Vector3d(point)) / 2.0;
                    const float radius = (somaRadius + point.w * 0.5) / 2.0;
                    geometryIndex =
                        _addSphere(useSdf, position, radius, baseMaterialId,
                                   NO_USER_DATA, *model, sdfMorphologyData,
                                   neighbours);
                    neighbours.insert(geometryIndex);

                    // Section connected to the soma
                    geometryIndex =
                        _addCone(useSdf, Vector3d(averagePoint),
                                 averagePoint.w * 0.75f,
                                 somaPosition + Vector3d(point), point.w * 0.5,
                                 baseMaterialId, NO_USER_DATA, *model,
                                 sdfMorphologyData, neighbours,
                                 DEFAULT_SOMA_DISPLACEMENT);
                    neighbours.insert(geometryIndex);
                }

                _addSection(*model, section.first, section.second,
                            geometryIndex, somaPosition, somaRadius,
                            baseMaterialId, mitochondriaDensity,
                            sdfMorphologyData, materialIds);
            }
        }
    }

    _createMaterials(materialIds, *model);

    if (useSdf)
        _finalizeSDFGeometries(*model, sdfMorphologyData);

    ModelMetadata metadata = {
        {"Number of Neurons", std::to_string(somas.size())}};

    _modelDescriptor.reset(new brayns::ModelDescriptor(std::move(model),
                                                       _details.assemblyName,
                                                       metadata));
    if (_modelDescriptor)
        _scene.addModel(_modelDescriptor);
    else
        PLUGIN_THROW("Neurons model could not be created");
}

void Neurons::_addSection(Model& model, const uint64_t sectionId,
                          const Section& section,
                          const size_t somaGeometryIndex,
                          const Vector3d& somaPosition, const double somaRadius,
                          const size_t baseMaterialId,
                          const double mitochondriaDensity,
                          SDFMorphologyData& sdfMorphologyData,
                          MaterialSet& materialIds)
{
    const auto sectionType = static_cast<NeuronSectionType>(section.type);
    auto useSdf = _details.useSdf;

    const size_t sectionMaterialId =
        baseMaterialId +
        (_details.morphologyColorScheme == MorphologyColorScheme::section
             ? section.type
             : 0);
    materialIds.insert(sectionMaterialId);

    const auto& points = section.points;
    if (sectionType == NeuronSectionType::axon && !_details.loadAxons)
        return;
    if (sectionType == NeuronSectionType::basal_dendrite &&
        !_details.loadBasalDendrites)
        return;
    if (sectionType == NeuronSectionType::apical_dendrite &&
        !_details.loadApicalDendrites)
        return;

    double sectionLength = 0.0;
    double sectionVolume = 0.0;
    uint64_t geometryIndex = 0;
    uint64_t startingPoint = (section.parentId == SOMA_AS_PARENT ? 1 : 0);
    for (uint64_t i = startingPoint; i < points.size() - 1; ++i)
    {
        const auto& srcPoint = points[i];
        const Vector3d src = somaPosition + Vector3d(srcPoint);
        const double srcRadius = srcPoint.w * 0.5;

        const auto& dstPoint = points[i + 1];
        const Vector3d dst = somaPosition + Vector3d(dstPoint);
        const double dstRadius = dstPoint.w * 0.5;
        const double sampleLength = length(dstPoint - srcPoint);
        sectionLength += sampleLength;

        if (!useSdf)
            _addSphere(useSdf, dst, dstRadius, sectionMaterialId, NO_USER_DATA,
                       model, sdfMorphologyData, {});

        Neighbours neighbours{somaGeometryIndex};
        if (i > 0)
            neighbours = {geometryIndex};
        geometryIndex =
            _addCone(useSdf, src, srcRadius, dst, dstRadius, sectionMaterialId,
                     NO_USER_DATA, model, sdfMorphologyData, neighbours,
                     DEFAULT_SECTION_DISPLACEMENT);
        sectionVolume += coneVolume(sampleLength, srcRadius, dstRadius);

        _bounds.merge(srcPoint);
    }

    if (sectionType == NeuronSectionType::axon)
    {
        if (_details.generateInternals)
            _addSectionInternals(somaPosition, sectionLength, sectionVolume,
                                 points, mitochondriaDensity, baseMaterialId,
                                 sdfMorphologyData, model);

        if (_details.generateExternals)
        {
            _addAxonMyelinSheath(somaPosition, sectionLength, points,
                                 mitochondriaDensity, baseMaterialId,
                                 sdfMorphologyData, model);
            materialIds.insert(baseMaterialId + MATERIAL_OFFSET_MYELIN_SHEATH);
        }
    }
}

size_t Neurons::_getNbMitochondrionSegments() const
{
    return 2 + rand() % 18;
}

void Neurons::_addSomaInternals(const uint64_t index, Model& model,
                                const size_t materialId,
                                const Vector3d& somaPosition,
                                const double somaRadius,
                                const double mitochondriaDensity,
                                SDFMorphologyData& sdfMorphologyData)
{
    const auto useSdf = _details.useSdf;

    // Constants
    const double nucleusDisplacementRatio = 2.0;
    const double mitochondrionRadiusRatio = 0.025;
    const double mitochondrionDisplacementRatio = 20.0;
    const double mitochondrionRadius =
        somaRadius * mitochondrionRadiusRatio; // 5% of the volume of the soma

    // Nucleus
    const double nucleusRadius =
        somaRadius * 0.7; // 70% of the volume of the soma;

    const double somaInnerRadius = nucleusRadius + mitochondrionRadius;
    const double somaOutterRadius = somaRadius * 0.9;
    const double availableVolumeForMitochondria =
        sphereVolume(somaRadius) * mitochondriaDensity;

    const size_t nucleusMaterialId = materialId + MATERIAL_OFFSET_NUCLEUS;
    _addSphere(useSdf, somaPosition, nucleusRadius, nucleusMaterialId,
               NO_USER_DATA, model, sdfMorphologyData, {},
               nucleusDisplacementRatio);

    // Mitochondria
    if (mitochondriaDensity == 0.0)
        return;

    const size_t mitochondrionMaterialId =
        materialId + MATERIAL_OFFSET_MITOCHONDRION;
    double mitochondriaVolume = 0.0;

    uint64_t geometryIndex = 0;
    while (mitochondriaVolume < availableVolumeForMitochondria)
    {
        const size_t nbSegments = _getNbMitochondrionSegments();
        const auto pointsInSphere =
            getPointsInSphere(nbSegments, somaInnerRadius / somaRadius);
        double previousRadius = mitochondrionRadius;
        for (size_t i = 0; i < nbSegments; ++i)
        {
            // Mitochondrion geometry
            const double radius =
                (1.0 + (rand() % 500 / 1000.0)) * mitochondrionRadius;
            const auto p2 = somaPosition + somaOutterRadius * pointsInSphere[i];

            Neighbours neighbours;
            if (i != 0)
                neighbours = {geometryIndex};
            geometryIndex =
                _addSphere(useSdf, p2, radius, mitochondrionMaterialId,
                           NO_USER_DATA, model, sdfMorphologyData, neighbours,
                           mitochondrionDisplacementRatio);

            mitochondriaVolume += sphereVolume(radius);

            if (i > 0)
            {
                const auto p1 =
                    somaPosition + somaOutterRadius * pointsInSphere[i - 1];
                geometryIndex =
                    _addCone(useSdf, p1, previousRadius, p2, radius,
                             mitochondrionMaterialId, NO_USER_DATA, model,
                             sdfMorphologyData, {geometryIndex},
                             mitochondrionDisplacementRatio);

                mitochondriaVolume +=
                    coneVolume(length(p2 - p1), previousRadius, radius);
            }
            previousRadius = radius;
        }
    }
}

void Neurons::_addSectionInternals(
    const Vector3d& somaPosition, const double sectionLength,
    const double sectionVolume, const Vector4fs& points,
    const double mitochondriaDensity, const size_t baseMaterialId,
    SDFMorphologyData& sdfMorphologyData, Model& model)
{
    if (mitochondriaDensity == 0.0)
        return;

    const auto useSdf = _details.useSdf;

    // Add mitochondria (density is per section, not for the full axon)
    const double mitochondrionSegmentSize = 0.25;
    const double mitochondrionRadiusRatio = 0.25;

    const size_t nbMaxMitochondrionSegments =
        sectionLength / mitochondrionSegmentSize;
    const double indexRatio =
        double(points.size()) / double(nbMaxMitochondrionSegments);

    double mitochondriaVolume = 0.0;
    const size_t mitochondrionMaterialId =
        baseMaterialId + MATERIAL_OFFSET_MITOCHONDRION;

    uint64_t nbSegments = _getNbMitochondrionSegments();
    int64_t mitochondrionSegment =
        -(rand() % (1 + nbMaxMitochondrionSegments / 10));
    double previousRadius;
    Vector3d previousPosition;

    uint64_t geometryIndex = 0;
    for (size_t step = 0; step < nbMaxMitochondrionSegments; ++step)
    {
        if (mitochondriaVolume < sectionVolume * mitochondriaDensity &&
            mitochondrionSegment >= 0 && mitochondrionSegment < nbSegments)
        {
            const uint64_t srcIndex = uint64_t(step * indexRatio);
            const uint64_t dstIndex = uint64_t(step * indexRatio) + 1;
            if (dstIndex < points.size())
            {
                const auto& srcSample = points[srcIndex];
                const auto& dstSample = points[dstIndex];
                const double srcRadius = srcSample.w * 0.5;
                const Vector3d srcPosition{
                    srcSample.x + srcRadius * (rand() % 100 - 50) / 500.0,
                    srcSample.y + srcRadius * (rand() % 100 - 50) / 500.0,
                    srcSample.z + srcRadius * (rand() % 100 - 50) / 500.0};
                const double dstRadius = dstSample.w * 0.5;
                const Vector3d dstPosition{
                    dstSample.x + dstRadius * (rand() % 100 - 50) / 500.0,
                    dstSample.y + dstRadius * (rand() % 100 - 50) / 500.0,
                    dstSample.z + dstRadius * (rand() % 100 - 50) / 500.0};

                const Vector3d direction = dstPosition - srcPosition;
                const Vector3d position =
                    srcPosition + direction * (step * indexRatio - srcIndex);
                const double mitocondrionRadius =
                    srcRadius * mitochondrionRadiusRatio;
                const double radius =
                    (1.0 + (rand() % 1000 - 500) / 1000.0) * mitocondrionRadius;

                Neighbours neighbours;
                if (mitochondrionSegment != 0)
                    neighbours = {geometryIndex};

                if (!useSdf)
                    _addSphere(useSdf, somaPosition + position, radius,
                               mitochondrionMaterialId, -1, model,
                               sdfMorphologyData, {});

                if (mitochondrionSegment > 0)
                {
                    Neighbours neighbours = {};
                    if (mitochondrionSegment > 1)
                        neighbours = {geometryIndex};
                    geometryIndex =
                        _addCone(useSdf, somaPosition + position, radius,
                                 somaPosition + previousPosition,
                                 previousRadius, mitochondrionMaterialId, -1,
                                 model, sdfMorphologyData, neighbours);

                    mitochondriaVolume +=
                        coneVolume(length(position - previousPosition), radius,
                                   previousRadius);
                }

                previousPosition = position;
                previousRadius = radius;
            }
        }
        ++mitochondrionSegment;

        if (mitochondrionSegment == nbSegments)
        {
            mitochondrionSegment =
                -(rand() % (1 + nbMaxMitochondrionSegments / 10));
            nbSegments = _getNbMitochondrionSegments();
        }
    }
}

void Neurons::_addAxonMyelinSheath(
    const Vector3d& somaPosition, const double sectionLength,
    const Vector4fs& points, const double mitochondriaDensity,
    const size_t materialId, SDFMorphologyData& sdfMorphologyData, Model& model)
{
    if (sectionLength == 0 || points.empty())
        return;

    const auto useSdf = _details.useSdf;

    const double myelinSteathLength = 10.0;
    const double myelinSteathRadius = 0.5;
    const double myelinSteathDisplacementRatio = 0.25;
    const size_t myelinSteathMaterialId =
        materialId + MATERIAL_OFFSET_MYELIN_SHEATH;

    if (sectionLength < 2 * myelinSteathLength)
        return;

    const uint64_t nbPoints = points.size();

    uint64_t i = NB_MYELIN_FREE_SEGMENTS; // Ignore first 3 segments
    while (i < nbPoints - NB_MYELIN_FREE_SEGMENTS)
    {
        // Start surrounding segments with myelin steaths
        const auto& srcPoint = points[i];
        const Vector3d srcPosition = somaPosition + Vector3d(srcPoint);
        if (!useSdf)
            _addSphere(useSdf, srcPosition, myelinSteathRadius,
                       myelinSteathMaterialId, NO_USER_DATA, model,
                       sdfMorphologyData, {});

        double currentLength = 0;
        Vector3d previousPosition = srcPosition;
        while (currentLength < myelinSteathLength &&
               i < nbPoints - NB_MYELIN_FREE_SEGMENTS)
        {
            ++i;
            const auto& dstPoint = points[i];
            const Vector3d dstPosition = somaPosition + Vector3d(dstPoint);
            currentLength += length(dstPosition - previousPosition);
            if (!useSdf)
                _addSphere(useSdf, dstPosition, myelinSteathRadius,
                           myelinSteathMaterialId, NO_USER_DATA, model,
                           sdfMorphologyData, {});
            _addCone(useSdf, dstPosition, myelinSteathRadius, previousPosition,
                     myelinSteathRadius, myelinSteathMaterialId, NO_USER_DATA,
                     model, sdfMorphologyData, {},
                     myelinSteathDisplacementRatio);
            previousPosition = dstPosition;
        }
        i += NB_MYELIN_FREE_SEGMENTS; // Leave free segments between myelin
                                      // steaths
    }
}

} // namespace morphology
} // namespace bioexplorer

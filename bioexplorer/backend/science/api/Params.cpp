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

#include "Params.h"

#include <common/json.hpp>

#if !defined(DOXYGEN_SHOULD_SKIP_THIS)

#ifndef PLATFORM_DEBUG_JSON_ENABLED
#define FROM_JSON(PARAM, JSON, NAME) PARAM.NAME = JSON[#NAME].get<decltype(PARAM.NAME)>()
#else
#define FROM_JSON(PARAM, JSON, NAME)                                         \
    try                                                                      \
    {                                                                        \
        PARAM.NAME = JSON[#NAME].get<decltype(PARAM.NAME)>();                \
    }                                                                        \
    catch (...)                                                              \
    {                                                                        \
        PLUGIN_ERROR("JSON parsing error for attribute '" << #NAME << "'!"); \
        throw;                                                               \
    }
#endif
#define TO_JSON(PARAM, JSON, NAME) JSON[#NAME] = PARAM.NAME

using namespace bioexplorer;
using namespace details;

std::string to_json(const Response &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, status);
        TO_JSON(param, js, contents);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

std::string to_json(const SceneInformationDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, nbModels);
        TO_JSON(param, js, nbMaterials);
        TO_JSON(param, js, nbSpheres);
        TO_JSON(param, js, nbCylinders);
        TO_JSON(param, js, nbCones);
        TO_JSON(param, js, nbVertices);
        TO_JSON(param, js, nbIndices);
        TO_JSON(param, js, nbNormals);
        TO_JSON(param, js, nbColors);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(GeneralSettingsDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, meshFolder);
        FROM_JSON(param, js, loggingLevel);
        FROM_JSON(param, js, databaseLoggingLevel);
        FROM_JSON(param, js, v1Compatibility);
        FROM_JSON(param, js, cacheEnabled);
        FROM_JSON(param, js, loadMorphologiesFromFileSystem);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(FocusOnDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, modelId);
        FROM_JSON(param, js, instanceId);
        FROM_JSON(param, js, direction);
        FROM_JSON(param, js, distance);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AssemblyDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, shape);
        FROM_JSON(param, js, shapeParams);
        FROM_JSON(param, js, shapeMeshContents);
        FROM_JSON(param, js, position);
        FROM_JSON(param, js, rotation);
        FROM_JSON(param, js, clippingPlanes);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const AssemblyDetails &payload)
{
    try
    {
        nlohmann::json js;

        TO_JSON(payload, js, name);
        TO_JSON(payload, js, position);
        TO_JSON(payload, js, rotation);
        TO_JSON(payload, js, clippingPlanes);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(AssemblyTransformationsDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, transformations);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(ProteinColorSchemeDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, colorScheme);
        FROM_JSON(param, js, palette);
        FROM_JSON(param, js, chainIds);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AminoAcidSequenceAsStringDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, sequence);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AminoAcidSequenceAsRangesDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, ranges);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AminoAcidInformationDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AminoAcidDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, index);
        FROM_JSON(param, js, aminoAcidShortName);
        FROM_JSON(param, js, chainIds);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(RNASequenceDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, pdbId);
        FROM_JSON(param, js, contents);
        FROM_JSON(param, js, proteinContents);
        FROM_JSON(param, js, shape);
        FROM_JSON(param, js, shapeParams);
        FROM_JSON(param, js, valuesRange);
        FROM_JSON(param, js, curveParams);
        FROM_JSON(param, js, atomRadiusMultiplier);
        FROM_JSON(param, js, representation);
        FROM_JSON(param, js, position);
        FROM_JSON(param, js, rotation);
        FROM_JSON(param, js, animationParams);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(MembraneDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, lipidPDBIds);
        FROM_JSON(param, js, lipidContents);
        FROM_JSON(param, js, lipidRotation);
        FROM_JSON(param, js, lipidDensity);
        FROM_JSON(param, js, atomRadiusMultiplier);
        FROM_JSON(param, js, loadBonds);
        FROM_JSON(param, js, loadNonPolymerChemicals);
        FROM_JSON(param, js, representation);
        FROM_JSON(param, js, chainIds);
        FROM_JSON(param, js, recenter);
        FROM_JSON(param, js, animationParams);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(ProteinDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, pdbId);
        FROM_JSON(param, js, contents);
        FROM_JSON(param, js, atomRadiusMultiplier);
        FROM_JSON(param, js, loadBonds);
        FROM_JSON(param, js, loadNonPolymerChemicals);
        FROM_JSON(param, js, loadHydrogen);
        FROM_JSON(param, js, representation);
        FROM_JSON(param, js, chainIds);
        FROM_JSON(param, js, recenter);
        FROM_JSON(param, js, transmembraneParams);
        FROM_JSON(param, js, occurrences);
        FROM_JSON(param, js, allowedOccurrences);
        FROM_JSON(param, js, animationParams);
        FROM_JSON(param, js, position);
        FROM_JSON(param, js, rotation);
        FROM_JSON(param, js, constraints);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const ProteinDetails &payload)
{
    try
    {
        nlohmann::json js;

        TO_JSON(payload, js, assemblyName);
        TO_JSON(payload, js, name);
        TO_JSON(payload, js, contents);
        TO_JSON(payload, js, atomRadiusMultiplier);
        TO_JSON(payload, js, loadBonds);
        TO_JSON(payload, js, loadNonPolymerChemicals);
        TO_JSON(payload, js, loadHydrogen);
        TO_JSON(payload, js, representation);
        TO_JSON(payload, js, chainIds);
        TO_JSON(payload, js, recenter);
        TO_JSON(payload, js, occurrences);
        TO_JSON(payload, js, allowedOccurrences);
        TO_JSON(payload, js, animationParams);
        TO_JSON(payload, js, position);
        TO_JSON(payload, js, rotation);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(SugarDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, pdbId);
        FROM_JSON(param, js, contents);
        FROM_JSON(param, js, proteinName);
        FROM_JSON(param, js, atomRadiusMultiplier);
        FROM_JSON(param, js, loadBonds);
        FROM_JSON(param, js, representation);
        FROM_JSON(param, js, recenter);
        FROM_JSON(param, js, chainIds);
        FROM_JSON(param, js, siteIndices);
        FROM_JSON(param, js, rotation);
        FROM_JSON(param, js, animationParams);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(EnzymeReactionDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, enzymeName);
        FROM_JSON(param, js, substrateNames);
        FROM_JSON(param, js, productNames);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(EnzymeReactionProgressDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, instanceId);
        FROM_JSON(param, js, progress);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AddGridDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, minValue);
        FROM_JSON(param, js, maxValue);
        FROM_JSON(param, js, steps);
        FROM_JSON(param, js, radius);
        FROM_JSON(param, js, planeOpacity);
        FROM_JSON(param, js, showAxis);
        FROM_JSON(param, js, showPlanes);
        FROM_JSON(param, js, showFullGrid);
        FROM_JSON(param, js, useColors);
        FROM_JSON(param, js, position);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AddSpheresDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, positions);
        FROM_JSON(param, js, radii);
        FROM_JSON(param, js, color);
        FROM_JSON(param, js, opacity);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AddConesDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, origins);
        FROM_JSON(param, js, targets);
        FROM_JSON(param, js, originsRadii);
        FROM_JSON(param, js, targetsRadii);
        FROM_JSON(param, js, color);
        FROM_JSON(param, js, opacity);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AddBoundingBoxDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, bottomLeft);
        FROM_JSON(param, js, topRight);
        FROM_JSON(param, js, radius);
        FROM_JSON(param, js, color);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AddBoxDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, bottomLeft);
        FROM_JSON(param, js, topRight);
        FROM_JSON(param, js, color);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AddStreamlinesDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, indices);
        FROM_JSON(param, js, vertices);
        FROM_JSON(param, js, colors);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(ModelIdDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, modelId);
        FROM_JSON(param, js, maxNbInstances);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AddModelInstanceDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, modelId);
        FROM_JSON(param, js, translation);
        FROM_JSON(param, js, rotation);
        FROM_JSON(param, js, rotationCenter);
        FROM_JSON(param, js, scale);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(SetModelInstancesDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, modelId);
        FROM_JSON(param, js, translations);
        FROM_JSON(param, js, rotations);
        FROM_JSON(param, js, rotationCenters);
        FROM_JSON(param, js, scales);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const ModelTransformationDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, translation);
        TO_JSON(param, js, rotation);
        TO_JSON(param, js, rotationCenter);
        TO_JSON(param, js, scale);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

std::string to_json(const ModelBoundsDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, minAABB);
        TO_JSON(param, js, maxAABB);
        TO_JSON(param, js, center);
        TO_JSON(param, js, size);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(MaterialsDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, modelIds);
        FROM_JSON(param, js, materialIds);
        FROM_JSON(param, js, diffuseColors);
        FROM_JSON(param, js, specularColors);
        FROM_JSON(param, js, specularExponents);
        FROM_JSON(param, js, reflectionIndices);
        FROM_JSON(param, js, opacities);
        FROM_JSON(param, js, refractionIndices);
        FROM_JSON(param, js, emissions);
        FROM_JSON(param, js, glossinesses);
        FROM_JSON(param, js, castUserData);
        FROM_JSON(param, js, shadingModes);
        FROM_JSON(param, js, userParameters);
        FROM_JSON(param, js, chameleonModes);
        FROM_JSON(param, js, clippingModes);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const IdsDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, ids);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(NameDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const NameDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, name);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

// Fields
bool from_json(BuildFieldsDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, voxelSize);
        FROM_JSON(param, js, density);
        FROM_JSON(param, js, dataType);
        FROM_JSON(param, js, modelIds);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(FileAccessDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, filename);
        FROM_JSON(param, js, lowBounds);
        FROM_JSON(param, js, highBounds);
        FROM_JSON(param, js, fileFormat);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(LASFileAccessDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, filename);
        FROM_JSON(param, js, modelIds);
        FROM_JSON(param, js, materialIds);
        FROM_JSON(param, js, exportColors);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(DatabaseAccessDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, brickId);
        FROM_JSON(param, js, lowBounds);
        FROM_JSON(param, js, highBounds);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(ModelIdFileAccessDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, modelId);
        FROM_JSON(param, js, filename);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(BuildPointCloudDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, radius);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(ModelLoadingTransactionDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, action);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(ProteinInstanceTransformationDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, instanceIndex);
        FROM_JSON(param, js, position);
        FROM_JSON(param, js, rotation);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(InspectionDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, origin);
        FROM_JSON(param, js, direction);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const ProteinInspectionDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, hit);
        TO_JSON(param, js, assemblyName);
        TO_JSON(param, js, proteinName);
        TO_JSON(param, js, modelId);
        TO_JSON(param, js, instanceId);
        TO_JSON(param, js, position);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(AtlasDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, loadCells);
        FROM_JSON(param, js, cellRadius);
        FROM_JSON(param, js, loadMeshes);
        FROM_JSON(param, js, cellSqlFilter);
        FROM_JSON(param, js, regionSqlFilter);
        FROM_JSON(param, js, scale);
        FROM_JSON(param, js, meshPosition);
        FROM_JSON(param, js, meshRotation);
        FROM_JSON(param, js, meshScale);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(VasculatureDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, colorScheme);
        FROM_JSON(param, js, realismLevel);
        FROM_JSON(param, js, gids);
        FROM_JSON(param, js, representation);
        FROM_JSON(param, js, radiusMultiplier);
        FROM_JSON(param, js, sqlFilter);
        FROM_JSON(param, js, scale);
        FROM_JSON(param, js, animationParams);
        FROM_JSON(param, js, displacementParams);
        FROM_JSON(param, js, alignToGrid);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(VasculatureReportDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, simulationReportId);
        FROM_JSON(param, js, showEvolution);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(VasculatureRadiusReportDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, simulationReportId);
        FROM_JSON(param, js, frame);
        FROM_JSON(param, js, amplitude);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(AstrocytesDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, vasculaturePopulationName);
        FROM_JSON(param, js, connectomePopulationName);
        FROM_JSON(param, js, loadSomas);
        FROM_JSON(param, js, loadDendrites);
        FROM_JSON(param, js, generateInternals);
        FROM_JSON(param, js, loadMicroDomains);
        FROM_JSON(param, js, realismLevel);
        FROM_JSON(param, js, morphologyRepresentation);
        FROM_JSON(param, js, microDomainRepresentation);
        FROM_JSON(param, js, morphologyColorScheme);
        FROM_JSON(param, js, populationColorScheme);
        FROM_JSON(param, js, radiusMultiplier);
        FROM_JSON(param, js, sqlFilter);
        FROM_JSON(param, js, scale);
        FROM_JSON(param, js, animationParams);
        FROM_JSON(param, js, displacementParams);
        FROM_JSON(param, js, maxDistanceToSoma);
        FROM_JSON(param, js, alignToGrid);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(NeuronsDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, loadSomas);
        FROM_JSON(param, js, loadAxon);
        FROM_JSON(param, js, loadBasalDendrites);
        FROM_JSON(param, js, loadApicalDendrites);
        FROM_JSON(param, js, synapsesType);
        FROM_JSON(param, js, generateInternals);
        FROM_JSON(param, js, generateExternals);
        FROM_JSON(param, js, showMembrane);
        FROM_JSON(param, js, generateVaricosities);
        FROM_JSON(param, js, realismLevel);
        FROM_JSON(param, js, morphologyRepresentation);
        FROM_JSON(param, js, morphologyColorScheme);
        FROM_JSON(param, js, populationColorScheme);
        FROM_JSON(param, js, radiusMultiplier);
        FROM_JSON(param, js, reportParams);
        FROM_JSON(param, js, sqlNodeFilter);
        FROM_JSON(param, js, sqlSectionFilter);
        FROM_JSON(param, js, scale);
        FROM_JSON(param, js, animationParams);
        FROM_JSON(param, js, displacementParams);
        FROM_JSON(param, js, maxDistanceToSoma);
        FROM_JSON(param, js, alignToGrid);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(NeuronIdSectionIdDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, neuronId);
        FROM_JSON(param, js, sectionId);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(NeuronIdDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, neuronId);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const NeuronPointsDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, status);
        TO_JSON(param, js, points);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(LookAtDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, source);
        FROM_JSON(param, js, target);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

std::string to_json(const LookAtResponseDetails &param)
{
    try
    {
        nlohmann::json js;
        TO_JSON(param, js, rotation);
        return js.dump();
    }
    catch (...)
    {
        return "";
    }
    return "";
}

bool from_json(WhiteMatterDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, radius);
        FROM_JSON(param, js, sqlFilter);
        FROM_JSON(param, js, scale);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(SynaptomeDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, radius);
        FROM_JSON(param, js, force);
        FROM_JSON(param, js, sqlNodeFilter);
        FROM_JSON(param, js, sqlEdgeFilter);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(SynapsesDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, radiusMultiplier);
        FROM_JSON(param, js, representation);
        FROM_JSON(param, js, realismLevel);
        FROM_JSON(param, js, sqlFilter);
        FROM_JSON(param, js, displacementParams);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(SynapseEfficacyDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, assemblyName);
        FROM_JSON(param, js, populationName);
        FROM_JSON(param, js, radius);
        FROM_JSON(param, js, sqlFilter);
        FROM_JSON(param, js, simulationReportId);
        FROM_JSON(param, js, alignToGrid);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(SpikeReportVisualizationSettingsDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, modelId);
        FROM_JSON(param, js, restVoltage);
        FROM_JSON(param, js, spikingVoltage);
        FROM_JSON(param, js, timeInterval);
        FROM_JSON(param, js, decaySpeed);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(bioexplorer::details::SDFTorusDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, position);
        FROM_JSON(param, js, outerRadius);
        FROM_JSON(param, js, innerRadius);
        FROM_JSON(param, js, displacement);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(bioexplorer::details::SDFVesicaDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, srcPosition);
        FROM_JSON(param, js, dstPosition);
        FROM_JSON(param, js, radius);
        FROM_JSON(param, js, displacement);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool from_json(bioexplorer::details::SDFEllipsoidDetails &param, const std::string &payload)
{
    try
    {
        auto js = nlohmann::json::parse(payload);
        FROM_JSON(param, js, name);
        FROM_JSON(param, js, position);
        FROM_JSON(param, js, radii);
        FROM_JSON(param, js, displacement);
    }
    catch (...)
    {
        return false;
    }
    return true;
}
#endif

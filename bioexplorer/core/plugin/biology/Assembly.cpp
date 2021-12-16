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

#include "Assembly.h"

#include <plugin/biology/Membrane.h>
#include <plugin/biology/Protein.h>
#include <plugin/biology/RNASequence.h>
#include <plugin/common/GeneralSettings.h>
#include <plugin/common/Logs.h>
#include <plugin/common/Utils.h>
#include <plugin/common/shapes/BezierShape.h>
#include <plugin/common/shapes/CubeShape.h>
#include <plugin/common/shapes/FanShape.h>
#include <plugin/common/shapes/MeshShape.h>
#include <plugin/common/shapes/PlaneShape.h>
#include <plugin/common/shapes/PointShape.h>
#include <plugin/common/shapes/Shape.h>
#include <plugin/common/shapes/SinusoidShape.h>
#include <plugin/common/shapes/SphereShape.h>

namespace bioexplorer
{
namespace biology
{
Assembly::Assembly(Scene &scene, const AssemblyDetails &details)
    : _details(details)
    , _scene(scene)
{
    const auto size = floatsToVector3f(details.shapeParams);
    _position = floatsToVector3f(_details.position);
    _rotation = floatsToQuaterniond(_details.rotation);
    _clippingPlanes = floatsToVector4fs(details.clippingPlanes);

    switch (details.shape)
    {
    case AssemblyShape::sphere:
    {
        _shape = ShapePtr(new SphereShape(_clippingPlanes, size.x));
        break;
    }
    case AssemblyShape::sinusoid:
    {
        _shape = ShapePtr(new SinusoidShape(_clippingPlanes, size));
        break;
    }
    case AssemblyShape::cube:
    {
        _shape = ShapePtr(new CubeShape(_clippingPlanes, size));
        break;
    }
    case AssemblyShape::fan:
    {
        _shape = ShapePtr(new FanShape(_clippingPlanes, size.x));
        break;
    }
    case AssemblyShape::plane:
    {
        _shape =
            ShapePtr(new PlaneShape(_clippingPlanes, Vector2f(size.x, size.y)));
        break;
    }
    case AssemblyShape::mesh:
    {
        _shape = ShapePtr(
            new MeshShape(size, _clippingPlanes, _details.shapeMeshContents));
        break;
    }
    default:
        _shape = ShapePtr(new PointShape(_clippingPlanes));
        break;
    }

    PLUGIN_INFO("Adding assembly [" << details.name << "] at position "
                                    << _position << ", rotation " << _rotation);
}

Assembly::~Assembly()
{
    for (const auto &protein : _proteins)
    {
        const auto modelId = protein.second->getModelDescriptor()->getModelID();
        PLUGIN_INFO("Removing protein [" << modelId << "] [" << protein.first
                                         << "] from assembly [" << _details.name
                                         << "]");
        _scene.removeModel(protein.second->getModelDescriptor()->getModelID());
    }
    if (_rnaSequence)
    {
        const auto modelId = _rnaSequence->getModelDescriptor()->getModelID();
        PLUGIN_INFO("Removing RNA sequence [" << modelId << "] from assembly ["
                                              << _details.name << "]");
        _scene.removeModel(modelId);
    }
    _modelDescriptors.clear();
}

void Assembly::addProtein(const ProteinDetails &details,
                          const AssemblyConstraints &constraints)
{
    ProteinPtr protein(new Protein(_scene, details));
    auto modelDescriptor = protein->getModelDescriptor();
    const auto randomParams =
        floatsToRandomizationDetails(details.randomParams);
    const auto proteinPosition = floatsToVector3f(details.position);
    const auto proteinRotation = floatsToQuaterniond(details.rotation);
    const auto transmembraneParams =
        floatsToVector2f(details.transmembraneParams);

    _processInstances(modelDescriptor, details.name, details.occurrences,
                      proteinPosition, proteinRotation,
                      details.allowedOccurrences, randomParams,
                      transmembraneParams.x, constraints);

    _proteins[details.name] = std::move(protein);
    _modelDescriptors.push_back(modelDescriptor);
    _scene.addModel(modelDescriptor);
    PLUGIN_INFO(
        "Number of instances: " << modelDescriptor->getInstances().size());
}

void Assembly::addMembrane(const MembraneDetails &details)
{
    if (_membrane)
        PLUGIN_THROW("Assembly already has a membrane");

    MembranePtr membrane(
        new Membrane(details, _scene, _position, _rotation, _shape, _proteins));
    _membrane = std::move(membrane);
}

void Assembly::addSugars(const SugarsDetails &details)
{
    // Get information from target protein (attributes, number of instances,
    // glycosylation sites, etc)
    const auto it = _proteins.find(details.proteinName);
    if (it == _proteins.end())
    {
        std::string s;
        for (const auto &protein : _proteins)
            s += "[" + protein.first + "]";
        PLUGIN_THROW("Target protein " + details.proteinName +
                     " not registered in assembly " + details.assemblyName +
                     ". Registered proteins are " + s);
    }
    PLUGIN_INFO("Adding sugars to protein " << details.proteinName);
    const auto targetProtein = (*it).second;
    targetProtein->addSugars(details);
}

void Assembly::addGlycans(const SugarsDetails &details)
{
    // Get information from target protein (attributes, number of instances,
    // glycosylation sites, etc)
    const auto it = _proteins.find(details.proteinName);
    if (it == _proteins.end())
    {
        std::string s;
        for (const auto &protein : _proteins)
            s += "[" + protein.first + "]";
        PLUGIN_THROW("Target protein " + details.proteinName +
                     " not registered in assembly " + details.assemblyName +
                     ". Registered proteins are " + s);
    }

    PLUGIN_INFO("Adding glycans to protein " << details.proteinName);
    const auto targetProtein = (*it).second;
    targetProtein->addGlycans(details);
}

void Assembly::_processInstances(
    ModelDescriptorPtr md, const std::string &name, const size_t occurrences,
    const Vector3f &position, const Quaterniond &rotation,
    const size_ts &allowedOccurrences, const RandomizationDetails &randDetails,
    const float offset, const AssemblyConstraints &constraints)
{
    srand(randDetails.seed);

    // Shape
    uint64_t count = 0;
    for (uint64_t occurence = 0; occurence < occurrences; ++occurence)
    {
        if (!allowedOccurrences.empty() &&
            std::find(allowedOccurrences.begin(), allowedOccurrences.end(),
                      occurence) == allowedOccurrences.end())
            continue;

        Transformations transformations;

        Transformation assemblyTransformation;
        assemblyTransformation.setTranslation(_position);
        assemblyTransformation.setRotation(_rotation);
        transformations.push_back(assemblyTransformation);

        Transformation shapeTransformation =
            _shape->getTransformation(occurence, occurrences, randDetails,
                                      offset);

        // TODO in shape
        if (isClipped(shapeTransformation.getTranslation(), _clippingPlanes))
            continue;

        transformations.push_back(shapeTransformation);

        Transformation proteinTransformation;
        proteinTransformation.setTranslation(position);
        proteinTransformation.setRotation(rotation);
        transformations.push_back(proteinTransformation);

        const Transformation finalTransformation =
            combineTransformations(transformations);
        const auto &translation = finalTransformation.getTranslation();

        // Assembly constaints
        bool addInstance = true;
        for (const auto &constraint : constraints)
        {
            if (constraint.first == AssemblyConstraintType::inside &&
                !constraint.second->isInside(translation))
                addInstance = false;
            if (constraint.first == AssemblyConstraintType::outside &&
                constraint.second->isInside(translation))
                addInstance = false;
        }
        if (!addInstance)
            continue;

        if (count == 0)
            md->setTransformation(finalTransformation);
        const ModelInstance instance(true, false, finalTransformation);
        md->addInstance(instance);

        ++count;
    }
}

void Assembly::setColorScheme(const ColorSchemeDetails &details)
{
    if (details.palette.size() < 3 || details.palette.size() % 3 != 0)
        PLUGIN_THROW("Invalid palette size");

    ProteinPtr protein{nullptr};
    const auto itProtein = _proteins.find(details.name);
    if (itProtein != _proteins.end())
        protein = (*itProtein).second;
    else if (_membrane)
    {
        const auto membraneLipids = _membrane->getLipids();
        const auto it =
            membraneLipids.find(details.assemblyName + '_' + details.name);
        if (it != membraneLipids.end())
            protein = (*it).second;
    }

    if (protein)
    {
        Palette palette;
        for (size_t i = 0; i < details.palette.size(); i += 3)
            palette.push_back({details.palette[i], details.palette[i + 1],
                               details.palette[i + 2]});

        PLUGIN_INFO("Applying color scheme to protein "
                    << details.name << " on assembly " << details.assemblyName);
        protein->setColorScheme(details.colorScheme, palette, details.chainIds);

        _scene.markModified();
    }
    else
        PLUGIN_ERROR("Protein " << details.name << " not found on assembly "
                                << details.assemblyName);
}

void Assembly::setAminoAcidSequenceAsString(
    const AminoAcidSequenceAsStringDetails &details)
{
    const auto it = _proteins.find(details.name);
    if (it != _proteins.end())
        (*it).second->setAminoAcidSequenceAsString(details.sequence);
    else
        PLUGIN_THROW("Protein not found: " + details.name);
}

void Assembly::setAminoAcidSequenceAsRange(
    const AminoAcidSequenceAsRangesDetails &details)
{
    const auto it = _proteins.find(details.name);
    if (it != _proteins.end())
    {
        Vector2uis ranges;
        for (size_t i = 0; i < details.ranges.size(); i += 2)
            ranges.push_back({details.ranges[i], details.ranges[i + 1]});

        (*it).second->setAminoAcidSequenceAsRanges(ranges);
    }
    else
        PLUGIN_THROW("Protein not found: " + details.name);
}

const std::string Assembly::getAminoAcidInformation(
    const AminoAcidInformationDetails &details) const
{
    PLUGIN_INFO("Returning Amino Acid information from protein "
                << details.name);

    std::string response;
    const auto it = _proteins.find(details.name);
    if (it != _proteins.end())
    {
        // Sequences
        for (const auto &sequence : (*it).second->getSequencesAsString())
        {
            if (!response.empty())
                response += "\n";
            response += sequence.second;
        }

        // Glycosylation sites
        const auto &sites = (*it).second->getGlycosylationSites({});
        for (const auto &site : sites)
        {
            std::string s;
            for (const auto &index : site.second)
            {
                if (!s.empty())
                    s += ",";
                s += std::to_string(index + 1); // Site indices start a 1, not 0
            }
            response += "\n" + s;
        }
    }
    else
        PLUGIN_THROW("Protein not found: " + details.name);

    return response;
}

void Assembly::setAminoAcid(const AminoAcidDetails &details)
{
    auto it = _proteins.find(details.name);
    if (it != _proteins.end())
        (*it).second->setAminoAcid(details);
    else
        PLUGIN_THROW("Protein not found: " + details.name);
}

void Assembly::addRNASequence(const RNASequenceDetails &details)
{
    auto rd = details;

    for (size_t i = 0; i < 3; ++i)
        rd.position[i] += _details.position[i];

    _rnaSequence =
        RNASequencePtr(new RNASequence(_scene, rd, _position, _rotation));
    const auto modelDescriptor = _rnaSequence->getModelDescriptor();
    _modelDescriptors.push_back(modelDescriptor);
    _scene.addModel(modelDescriptor);
    auto protein = _rnaSequence->getProtein();
    if (protein)
    {
        const auto name = protein->getDescriptor().name;
        _proteins[name] = std::move(protein);
    }
}

void Assembly::setProteinInstanceTransformation(
    const ProteinInstanceTransformationDetails &details)
{
    ProteinPtr protein{nullptr};
    const auto itProtein = _proteins.find(details.name);
    if (itProtein != _proteins.end())
        protein = (*itProtein).second;
    else
        PLUGIN_THROW("Protein " + details.name + " not found on assembly " +
                     details.assemblyName);

    const auto modelDescriptor = protein->getModelDescriptor();

    const auto &instances = modelDescriptor->getInstances();
    if (details.instanceIndex >= instances.size())
        PLUGIN_THROW("Invalid instance index (" +
                     std::to_string(details.instanceIndex) + ") for protein " +
                     details.name + " in assembly " + details.assemblyName);

    const auto instance = modelDescriptor->getInstance(details.instanceIndex);
    const auto &transformation = instance->getTransformation();

    const auto position = floatsToVector3f(details.position);
    const auto rotation = floatsToQuaterniond(details.rotation);

    PLUGIN_INFO("Modifying instance "
                << details.instanceIndex << " of protein " << details.name
                << " in assembly " << details.assemblyName << " with position="
                << position << " and rotation=" << rotation);
    Transformation newTransformation = transformation;
    newTransformation.setTranslation(position);
    newTransformation.setRotation(rotation);
    instance->setTransformation(newTransformation);
    if (details.instanceIndex == 0)
        modelDescriptor->setTransformation(newTransformation);

    _scene.markModified();
}

const Transformation Assembly::getProteinInstanceTransformation(
    const ProteinInstanceTransformationDetails &details) const
{
    ProteinPtr protein{nullptr};
    const auto itProtein = _proteins.find(details.name);
    if (itProtein != _proteins.end())
        protein = (*itProtein).second;
    else
        PLUGIN_THROW("Protein " + details.name + " not found on assembly " +
                     details.assemblyName);

    const auto modelDescriptor = protein->getModelDescriptor();

    const auto &instances = modelDescriptor->getInstances();
    if (details.instanceIndex >= instances.size())
        PLUGIN_THROW("Invalid instance index (" +
                     std::to_string(details.instanceIndex) + ") for protein " +
                     details.name + " in assembly " + details.assemblyName);

    const auto instance = modelDescriptor->getInstance(details.instanceIndex);
    auto transformation = instance->getTransformation();

    if (details.instanceIndex == 0)
        transformation = modelDescriptor->getTransformation();
    return transformation;
}

bool Assembly::isInside(const Vector3f &location) const
{
    bool result = false;
    if (_shape)
        result |= _shape->isInside(location);
    return result;
}
} // namespace biology
} // namespace bioexplorer

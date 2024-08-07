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

#include "Assembly.h"

#include <Defines.h>

#include <science/atlas/Atlas.h>
#include <science/common/GeneralSettings.h>
#include <science/common/Logs.h>
#include <science/common/Node.h>
#include <science/common/Utils.h>
#include <science/common/shapes/BezierShape.h>
#include <science/common/shapes/CubeShape.h>
#include <science/common/shapes/FanShape.h>
#include <science/common/shapes/HelixShape.h>
#include <science/common/shapes/PlaneShape.h>
#include <science/common/shapes/PointShape.h>
#include <science/common/shapes/SinusoidShape.h>
#include <science/common/shapes/SphereShape.h>
#include <science/common/shapes/SphericalCellDiffusionShape.h>
#include <science/connectomics/synapses/SynapseEfficacy.h>
#include <science/connectomics/synapses/SynapseEfficacySimulationHandler.h>
#include <science/connectomics/synaptome/Synaptome.h>
#include <science/connectomics/whitematter/WhiteMatter.h>
#include <science/molecularsystems/EnzymeReaction.h>
#include <science/molecularsystems/Membrane.h>
#include <science/molecularsystems/Protein.h>
#include <science/molecularsystems/RNASequence.h>
#include <science/morphologies/Astrocytes.h>
#include <science/morphologies/Neurons.h>
#include <science/morphologies/Synapses.h>
#include <science/vasculature/Vasculature.h>
#include <science/vasculature/VasculatureHandler.h>

#ifdef USE_ASSIMP
#include <science/common/shapes/MeshShape.h>
#endif

#include <platform/core/engineapi/Model.h>

using namespace core;

namespace bioexplorer
{
using namespace details;
using namespace morphology;
using namespace connectomics;
using namespace vasculature;
using namespace molecularsystems;
using namespace atlas;

namespace common
{
Assembly::Assembly(Scene &scene, const AssemblyDetails &details)
    : _details(details)
    , _scene(scene)
{
    const auto size = doublesToVector3d(details.shapeParams);
    _position = doublesToVector3d(_details.position);
    _rotation = doublesToQuaterniond(_details.rotation);
    _clippingPlanes = doublesToVector4ds(details.clippingPlanes);

    switch (details.shape)
    {
    case AssemblyShape::empty_sphere:
    case AssemblyShape::filled_sphere:
    {
        _shape = ShapePtr(new SphereShape(details.shape == AssemblyShape::filled_sphere, _clippingPlanes, size.x));
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
        _shape = ShapePtr(new PlaneShape(_clippingPlanes, Vector2f(size.x, size.z)));
        break;
    }
    case AssemblyShape::mesh:
#ifdef USE_ASSIMP
    {
        _shape = ShapePtr(new MeshShape(_clippingPlanes, size, _details.shapeMeshContents));
        break;
    }
#else
        PLUGIN_THROW("BioExplorer was not compiled with the Assimp library");
#endif
    case AssemblyShape::helix:
    {
        _shape = ShapePtr(new HelixShape(_clippingPlanes, size.x, size.y));
        break;
    }
    case AssemblyShape::spherical_cell_diffusion:
    {
        _shape = ShapePtr(new SphericalCellDiffusionShape(_clippingPlanes, size.x, size.y, size.z));
        break;
    }
    default:
        _shape = ShapePtr(new PointShape(_clippingPlanes));
        break;
    }

    PLUGIN_INFO(3, "Adding assembly [" << details.name << "] at position " << _position << ", rotation " << _rotation);
}

Assembly::~Assembly()
{
    for (const auto &protein : _proteins)
    {
        const auto modelId = protein.second->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing protein [" << modelId << "] [" << protein.first << "] from assembly [" << _details.name
                                            << "]");
        _scene.removeModel(protein.second->getModelDescriptor()->getModelID());
    }
    if (_rnaSequence)
    {
        const auto modelId = _rnaSequence->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing RNA sequence [" << modelId << "] from assembly [" << _details.name << "]");
        _scene.removeModel(modelId);
    }
    if (_vasculature)
    {
        const auto modelId = _vasculature->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing Vasculature [" << modelId << "] from assembly [" << _details.name << "]");
        _scene.removeModel(modelId);
    }
    if (_neurons)
    {
        const auto modelId = _neurons->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing Neurons [" << modelId << "] from assembly [" << _details.name << "]");
        _scene.removeModel(modelId);
    }
    if (_astrocytes)
    {
        const auto modelId = _astrocytes->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing Astrocytes [" << modelId << "] from assembly [" << _details.name << "]");
        _scene.removeModel(modelId);
    }
    if (_atlas)
    {
        const auto modelId = _atlas->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing Atlas [" << modelId << "] from assembly [" << _details.name << "]");
        _scene.removeModel(modelId);
    }
    if (_synapseEfficacy)
    {
        const auto modelId = _synapseEfficacy->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing synapse efficacy [" << modelId << "] from assembly [" << _details.name << "]");
        _scene.removeModel(modelId);
    }
    if (_synaptome)
    {
        const auto modelId = _synaptome->getModelDescriptor()->getModelID();
        PLUGIN_INFO(3, "Removing synapse efficacy [" << modelId << "] from assembly [" << _details.name << "]");
        _scene.removeModel(modelId);
    }
    _modelDescriptors.clear();
}

void Assembly::addProtein(const ProteinDetails &details, const AssemblyConstraints &constraints)
{
    ProteinPtr protein(new Protein(_scene, details));
    auto modelDescriptor = protein->getModelDescriptor();
    const auto animationParams = doublesToMolecularSystemAnimationDetails(details.animationParams);
    const auto proteinPosition = doublesToVector3d(details.position);
    const auto proteinRotation = doublesToQuaterniond(details.rotation);
    const auto transmembraneParams = doublesToVector2d(details.transmembraneParams);

    _processInstances(modelDescriptor, details.name, details.occurrences, proteinPosition, proteinRotation,
                      details.allowedOccurrences, animationParams, transmembraneParams.x, constraints);

    _proteins[details.name] = std::move(protein);
    _modelDescriptors.push_back(modelDescriptor);
    _scene.addModel(modelDescriptor);
    PLUGIN_INFO(3, "Number of instances: " << modelDescriptor->getInstances().size());
}

void Assembly::addMembrane(const MembraneDetails &details)
{
    if (_membrane)
        PLUGIN_THROW("Assembly already has a membrane");

    MembranePtr membrane(new Membrane(details, _scene, _position, _rotation, _shape, _proteins));
    _membrane = std::move(membrane);
}

void Assembly::addSugar(const SugarDetails &details)
{
    // Get information from target protein (attributes, number of instances, glycosylation sites, etc)
    const auto it = _proteins.find(details.proteinName);
    if (it == _proteins.end())
    {
        std::string s;
        for (const auto &protein : _proteins)
            s += "[" + protein.first + "]";
        PLUGIN_THROW("Target protein " + details.proteinName + " not registered in assembly " + details.assemblyName +
                     ". Registered proteins are " + s);
    }
    PLUGIN_INFO(3, "Adding sugars to protein " << details.proteinName);
    const auto targetProtein = (*it).second;
    targetProtein->addSugar(details);
}

void Assembly::addGlycan(const SugarDetails &details)
{
    // Get information from target protein (attributes, number of instances, glycosylation sites, etc)
    const auto it = _proteins.find(details.proteinName);
    if (it == _proteins.end())
    {
        std::string s;
        for (const auto &protein : _proteins)
            s += "[" + protein.first + "]";
        PLUGIN_THROW("Target protein " + details.proteinName + " not registered in assembly " + details.assemblyName +
                     ". Registered proteins are " + s);
    }

    PLUGIN_INFO(3, "Adding glycans to protein " << details.proteinName);
    const auto targetProtein = (*it).second;
    targetProtein->addGlycan(details);
}

void Assembly::_processInstances(ModelDescriptorPtr md, const std::string &name, const size_t occurrences,
                                 const Vector3d &position, const Quaterniond &rotation,
                                 const uint64_ts &allowedOccurrences,
                                 const MolecularSystemAnimationDetails &molecularSystemAnimationDetails,
                                 const double offset, const AssemblyConstraints &constraints)
{
    srand(molecularSystemAnimationDetails.seed);

    // Shape
    uint64_t count = 0;
    for (uint64_t occurrence = 0; occurrence < occurrences; ++occurrence)
    {
        try
        {
            if (!allowedOccurrences.empty() &&
                std::find(allowedOccurrences.begin(), allowedOccurrences.end(), occurrence) == allowedOccurrences.end())
                continue;

            Transformations transformations;

            Transformation assemblyTransformation;
            assemblyTransformation.setTranslation(_position);
            assemblyTransformation.setRotation(_rotation);
            transformations.push_back(assemblyTransformation);

            Transformation shapeTransformation =
                _shape->getTransformation(occurrence, occurrences, molecularSystemAnimationDetails, offset);

            transformations.push_back(shapeTransformation);

            const Transformation finalTransformation = combineTransformations(transformations);
            const auto &translation = finalTransformation.getTranslation();

            // Assembly constraints
            bool addInstance = true;
            for (const auto &constraint : constraints)
            {
                if (constraint.first == AssemblyConstraintType::inside && !constraint.second->isInside(translation))
                    addInstance = false;
                if (constraint.first == AssemblyConstraintType::outside && constraint.second->isInside(translation))
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
        catch (const std::runtime_error &)
        {
            // Instance is clipped
        }
    }
    md->computeBounds();
}

void Assembly::setProteinColorScheme(const ProteinColorSchemeDetails &details)
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
        const auto it = membraneLipids.find(details.assemblyName + '_' + details.name);
        if (it != membraneLipids.end())
            protein = (*it).second;
    }

    if (protein)
    {
        Palette palette;
        for (size_t i = 0; i < details.palette.size(); i += 3)
            palette.push_back({details.palette[i], details.palette[i + 1], details.palette[i + 2]});

        PLUGIN_INFO(3, "Applying color scheme to protein " << details.name << " on assembly " << details.assemblyName);
        protein->setColorScheme(details.colorScheme, palette, details.chainIds);

        _scene.markModified();
    }
    else
        PLUGIN_ERROR("Protein " << details.name << " not found on assembly " << details.assemblyName);
}

void Assembly::setAminoAcidSequenceAsString(const AminoAcidSequenceAsStringDetails &details)
{
    const auto it = _proteins.find(details.name);
    if (it != _proteins.end())
        (*it).second->setAminoAcidSequenceAsString(details.sequence);
    else
        PLUGIN_THROW("Protein not found: " + details.name);
}

void Assembly::setAminoAcidSequenceAsRange(const AminoAcidSequenceAsRangesDetails &details)
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

const std::string Assembly::getAminoAcidInformation(const AminoAcidInformationDetails &details) const
{
    PLUGIN_INFO(3, "Returning Amino Acid information from protein " << details.name);

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

    for (size_t i = 0; i < _details.position.size(); ++i)
        rd.position[i] += _details.position[i];

    _rnaSequence = RNASequencePtr(new RNASequence(_scene, rd, _clippingPlanes, _position, _rotation));
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

void Assembly::setProteinInstanceTransformation(const ProteinInstanceTransformationDetails &details)
{
    ProteinPtr protein{nullptr};
    const auto itProtein = _proteins.find(details.name);
    if (itProtein != _proteins.end())
        protein = (*itProtein).second;
    else
        PLUGIN_THROW("Protein " + details.name + " not found on assembly " + details.assemblyName);

    const auto modelDescriptor = protein->getModelDescriptor();

    const auto &instances = modelDescriptor->getInstances();
    if (details.instanceIndex >= instances.size())
        PLUGIN_THROW("Invalid instance index (" + std::to_string(details.instanceIndex) + ") for protein " +
                     details.name + " in assembly " + details.assemblyName);

    const auto instance = modelDescriptor->getInstance(details.instanceIndex);
    const auto &transformation = instance->getTransformation();

    const auto position = doublesToVector3d(details.position);
    const auto rotation = doublesToQuaterniond(details.rotation);

    PLUGIN_INFO(3, "Modifying instance " << details.instanceIndex << " of protein " << details.name << " in assembly "
                                         << details.assemblyName << " with position=" << position
                                         << " and rotation=" << rotation);
    Transformation newTransformation = transformation;
    newTransformation.setTranslation(position);
    newTransformation.setRotation(rotation);
    if (details.instanceIndex == 0)
        modelDescriptor->setTransformation(newTransformation);
    instance->setTransformation(newTransformation);

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
        PLUGIN_THROW("Protein " + details.name + " not found on assembly " + details.assemblyName);

    const auto modelDescriptor = protein->getModelDescriptor();

    const auto &instances = modelDescriptor->getInstances();
    if (details.instanceIndex >= instances.size())
        PLUGIN_THROW("Invalid instance index (" + std::to_string(details.instanceIndex) + ") for protein " +
                     details.name + " in assembly " + details.assemblyName);

    const auto instance = modelDescriptor->getInstance(details.instanceIndex);
    const auto transformation = instance->getTransformation();
    const auto &position = transformation.getTranslation();
    const auto &rotation = transformation.getRotation();

    PLUGIN_INFO(3, "Getting instance " << details.instanceIndex << " of protein " << details.name << " in assembly "
                                       << details.assemblyName << " with position=" << position
                                       << " and rotation=" << rotation);

    return transformation;
}

bool Assembly::isInside(const Vector3d &location) const
{
    bool result = false;
    if (_shape)
        result |= _shape->isInside(location);
    return result;
}

ProteinInspectionDetails Assembly::inspect(const Vector3d &origin, const Vector3d &direction, double &t) const
{
    ProteinInspectionDetails result;
    result.hit = false;
    result.assemblyName = _details.name;

    t = std::numeric_limits<double>::max();

    // Proteins
    for (const auto protein : _proteins)
    {
        const auto md = protein.second->getModelDescriptor();
        const auto &instances = md->getInstances();
        const Vector3d instanceHalfSize = protein.second->getBounds().getSize() / 2.0;

        uint64_t count = 0;
        for (const auto &instance : instances)
        {
            const auto instancePosition = instance.getTransformation().getTranslation();

            Boxd box;
            box.merge(instancePosition - instanceHalfSize);
            box.merge(instancePosition + instanceHalfSize);

            double tHit;
            if (rayBoxIntersection(origin, direction, box, 0.0, t, tHit))
            {
                result.hit = true;
                if (tHit < t)
                {
                    result.proteinName = protein.second->getDescriptor().name;
                    result.modelId = md->getModelID();
                    result.instanceId = count;
                    result.position = {instancePosition.x, instancePosition.y, instancePosition.z};
                    t = tHit;
                }
            }
            ++count;
        }
    }

    // Membrane
    if (_membrane)
    {
        for (const auto protein : _membrane->getLipids())
        {
            const auto md = protein.second->getModelDescriptor();
            const auto &instances = md->getInstances();
            const Vector3d instanceHalfSize = protein.second->getBounds().getSize() / 2.0;

            uint64_t count = 0;
            for (const auto &instance : instances)
            {
                const auto instancePosition = instance.getTransformation().getTranslation();

                Boxd box;
                box.merge(instancePosition - instanceHalfSize);
                box.merge(instancePosition + instanceHalfSize);

                double tHit;
                if (rayBoxIntersection(origin, direction, box, 0.0, t, tHit))
                {
                    result.hit = true;
                    if (tHit < t)
                    {
                        result.proteinName = protein.second->getDescriptor().name;
                        result.modelId = md->getModelID();
                        result.instanceId = count;
                        result.position = {instancePosition.x, instancePosition.y, instancePosition.z};
                        t = tHit;
                    }
                }
                ++count;
            }
        }
    }

    return result;
}

void Assembly::addVasculature(const VasculatureDetails &details)
{
    if (_vasculature)
    {
        auto modelDescriptor = _vasculature->getModelDescriptor();
        if (modelDescriptor)
        {
            const auto modelId = modelDescriptor->getModelID();
            _scene.removeModel(modelId);
        }
    }
    _vasculature.reset(std::move(new Vasculature(_scene, details, _position, _rotation)));
    _scene.addModel(_vasculature->getModelDescriptor());
    _scene.markModified(false);
}

std::string Assembly::getVasculatureInfo() const
{
    auto modelDescriptor = _vasculature->getModelDescriptor();
    Response response;
    if (!_vasculature)
        PLUGIN_THROW("No vasculature is currently defined in assembly " + _details.name);
    std::stringstream s;
    s << "modelId=" << modelDescriptor->getModelID() << CONTENTS_DELIMITER << "nbNodes=" << _vasculature->getNbNodes();
    return s.str().c_str();
}

void Assembly::setVasculatureReport(const VasculatureReportDetails &details)
{
    PLUGIN_INFO(3, "Setting report to vasculature");
    if (!_vasculature)
        PLUGIN_THROW("No vasculature is currently loaded");

    auto modelDescriptor = _vasculature->getModelDescriptor();
    auto handler = std::make_shared<VasculatureHandler>(details);
    auto &model = modelDescriptor->getModel();
    model.setSimulationHandler(handler);
}

void Assembly::setVasculatureRadiusReport(const VasculatureRadiusReportDetails &details)
{
    if (!_vasculature && !_astrocytes)
        PLUGIN_THROW("No vasculature nor astrocytes are currently loaded");

    if (_vasculature)
        _vasculature->setRadiusReport(details);
    if (_astrocytes)
        _astrocytes->setVasculatureRadiusReport(details);
}

void Assembly::addAstrocytes(const AstrocytesDetails &details)
{
    if (_astrocytes)
        PLUGIN_THROW("Astrocytes already exists in assembly " + details.assemblyName);

    _astrocytes.reset(new Astrocytes(_scene, details, _position, _rotation));
    _scene.addModel(_astrocytes->getModelDescriptor());
    _scene.markModified(false);
}

void Assembly::addAtlas(const AtlasDetails &details)
{
    if (_atlas)
        PLUGIN_THROW("Atlas already exists in assembly " + details.assemblyName);

    _atlas.reset(new Atlas(_scene, details, _position, _rotation));
    _scene.addModel(_atlas->getModelDescriptor());
    _scene.markModified(false);
}

void Assembly::addNeurons(const NeuronsDetails &details)
{
    if (_neurons)
        PLUGIN_THROW("Neurons already exists in assembly " + details.assemblyName);

    _neurons.reset(new Neurons(_scene, details, _position, _rotation));
    _scene.addModel(_neurons->getModelDescriptor());
    _scene.markModified(false);
}

void Assembly::addSynaptome(const SynaptomeDetails &details)
{
    if (_synaptome)
        PLUGIN_THROW("Synaptome already exists in assembly " + details.assemblyName);

    _synaptome.reset(new Synaptome(_scene, details, _position, _rotation));
    _scene.addModel(_synaptome->getModelDescriptor());
    _scene.markModified(false);
}

Vector4ds Assembly::getNeuronSectionPoints(const NeuronIdSectionIdDetails &details)
{
    if (!_neurons)
        PLUGIN_THROW("No neurons are currently defined in assembly " + details.assemblyName);
    return _neurons->getNeuronSectionPoints(details.neuronId, details.sectionId);
}

Vector3ds Assembly::getNeuronVaricosities(const NeuronIdDetails &details)
{
    if (!_neurons)
        PLUGIN_THROW("No neurons are currently defined in assembly " + details.assemblyName);
    return _neurons->getNeuronVaricosities(details.neuronId);
}

ProteinPtr Assembly::getProtein(const std::string &name)
{
    ProteinPtr protein{nullptr};
    const auto it = _proteins.find(name);
    if (it != _proteins.end())
        protein = (*it).second;
    return protein;
}

Transformation Assembly::getTransformation() const
{
    Transformation transformation;
    transformation.setTranslation(doublesToVector3d(_details.position));
    transformation.setRotation(doublesToQuaterniond(_details.rotation));
    return transformation;
}

void Assembly::addEnzymeReaction(const EnzymeReactionDetails &details, AssemblyPtr enzymeAssembly, ProteinPtr enzyme,
                                 Proteins &substrates, Proteins &products)
{
    auto enzymeReaction =
        EnzymeReactionPtr(new EnzymeReaction(_scene, details, enzymeAssembly, enzyme, substrates, products));
    _enzymeReactions[details.name] = enzymeReaction;
}

void Assembly::setEnzymeReactionProgress(const EnzymeReactionProgressDetails &details)
{
    if (_enzymeReactions.find(details.name) == _enzymeReactions.end())
        PLUGIN_THROW("Enzyme reaction does not exist in assembly " + _details.name);
    _enzymeReactions[details.name]->setProgress(details.instanceId, details.progress);
}

void Assembly::addWhiteMatter(const WhiteMatterDetails &details)
{
    if (_whiteMatter)
    {
        auto modelDescriptor = _whiteMatter->getModelDescriptor();
        if (modelDescriptor)
        {
            const auto modelId = modelDescriptor->getModelID();
            _scene.removeModel(modelId);
        }
    }
    _whiteMatter.reset(new WhiteMatter(_scene, details, _position, _rotation));
    _scene.addModel(_whiteMatter->getModelDescriptor());
    _scene.markModified(false);
}

void Assembly::addSynapses(const SynapsesDetails &details)
{
    if (_synapses)
    {
        auto modelDescriptor = _synapses->getModelDescriptor();
        if (modelDescriptor)
        {
            const auto modelId = modelDescriptor->getModelID();
            _scene.removeModel(modelId);
        }
    }
    _synapses.reset(std::move(new Synapses(_scene, details, _position, _rotation)));
    _scene.markModified(false);
}

void Assembly::addSynapseEfficacy(const SynapseEfficacyDetails &details)
{
    if (_synapseEfficacy)
    {
        auto modelDescriptor = _synapseEfficacy->getModelDescriptor();
        if (modelDescriptor)
        {
            const auto modelId = modelDescriptor->getModelID();
            _scene.removeModel(modelId);
        }
    }
    _synapseEfficacy.reset(std::move(new SynapseEfficacy(_scene, details, _position, _rotation)));

    auto modelDescriptor = _synapseEfficacy->getModelDescriptor();
    auto handler = std::make_shared<SynapseEfficacySimulationHandler>(details);
    auto &model = modelDescriptor->getModel();
    setDefaultTransferFunction(model, Vector2d(0.0, 1.0), 1.0);
    model.setSimulationHandler(handler);

    _scene.markModified(false);
}

} // namespace common
} // namespace bioexplorer

/*
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
 *
 * Copyright 2020-2023 Blue BrainProject / EPFL
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

#include <science/common/Types.h>

using namespace bioexplorer;
using namespace details;

#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
// Response
std::string to_json(const Response &param);

// Settings
bool from_json(GeneralSettingsDetails &param, const std::string &payload);

// Scene information
std::string to_json(const SceneInformationDetails &param);

// Camera
bool from_json(FocusOnDetails &param, const std::string &payload);

// Biological elements
bool from_json(AssemblyDetails &param, const std::string &payload);
std::string to_json(const AssemblyDetails &payload);

bool from_json(AssemblyTransformationsDetails &param, const std::string &payload);

bool from_json(RNASequenceDetails &param, const std::string &payload);

bool from_json(MembraneDetails &param, const std::string &payload);

bool from_json(ProteinDetails &param, const std::string &payload);
std::string to_json(const ProteinDetails &payload);

bool from_json(SugarDetails &param, const std::string &payload);

// Enzyme reactions
bool from_json(EnzymeReactionDetails &param, const std::string &payload);
bool from_json(EnzymeReactionProgressDetails &param, const std::string &payload);

// Other elements
bool from_json(AddGridDetails &param, const std::string &payload);
bool from_json(AddSpheresDetails &param, const std::string &payload);
bool from_json(AddConeDetails &param, const std::string &payload);
bool from_json(AddBoundingBoxDetails &param, const std::string &payload);
bool from_json(AddBoxDetails &param, const std::string &payload);
bool from_json(AddStreamlinesDetails &param, const std::string &payload);

// Amino acids
bool from_json(AminoAcidSequenceAsStringDetails &param, const std::string &payload);
bool from_json(AminoAcidSequenceAsRangesDetails &param, const std::string &payload);
bool from_json(AminoAcidInformationDetails &param, const std::string &payload);
bool from_json(AminoAcidDetails &param, const std::string &payload);

// Files
bool from_json(FileAccessDetails &param, const std::string &payload);

// DB
bool from_json(DatabaseAccessDetails &param, const std::string &payload);

// Models, Color schemes and materials
bool from_json(ProteinColorSchemeDetails &param, const std::string &payload);
bool from_json(ModelIdDetails &modelId, const std::string &payload);
bool from_json(MaterialsDetails &materialsDetails, const std::string &payload);
std::string to_json(const IdsDetails &param);
bool from_json(NameDetails &param, const std::string &payload);
std::string to_json(const NameDetails &param);

// Fields
bool from_json(BuildFieldsDetails &param, const std::string &payload);
bool from_json(ModelIdFileAccessDetails &param, const std::string &payload);

// Point cloud
bool from_json(BuildPointCloudDetails &param, const std::string &payload);

// Models and instances
bool from_json(ModelLoadingTransactionDetails &param, const std::string &payload);
bool from_json(ProteinInstanceTransformationDetails &param, const std::string &payload);

// Protein inspection
bool from_json(InspectionDetails &param, const std::string &payload);
std::string to_json(const ProteinInspectionDetails &param);

// Vasculature
bool from_json(VasculatureDetails &param, const std::string &payload);
bool from_json(VasculatureReportDetails &param, const std::string &payload);
bool from_json(VasculatureRadiusReportDetails &param, const std::string &payload);

bool from_json(AtlasDetails &param, const std::string &payload);
bool from_json(AstrocytesDetails &param, const std::string &payload);
bool from_json(NeuronsDetails &param, const std::string &payload);
bool from_json(NeuronIdSectionIdDetails &param, const std::string &payload);
bool from_json(NeuronIdDetails &param, const std::string &payload);
std::string to_json(const NeuronPointsDetails &param);

// Connectomics
bool from_json(WhiteMatterDetails &param, const std::string &payload);
bool from_json(SynapsesDetails &param, const std::string &payload);
bool from_json(SynapseEfficacyDetails &param, const std::string &payload);
bool from_json(SpikeReportVisualizationSettingsDetails &param, const std::string &payload);

// Utilities
bool from_json(LookAtDetails &param, const std::string &payload);
std::string to_json(const LookAtResponseDetails &param);
#endif
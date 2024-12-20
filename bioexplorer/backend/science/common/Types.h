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

#pragma once

#include <Defines.h>

#include <platform/core/common/geometry/SDFGeometry.h>
#include <platform/core/engineapi/Scene.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace bioexplorer
{
const std::string CONTENTS_DELIMITER = "||||";

// Metadata
const std::string METADATA_ASSEMBLY = "Assembly";
const std::string METADATA_PDB_ID = "PDBId";
const std::string METADATA_HEADER = "Header";
const std::string METADATA_ATOMS = "Atoms";
const std::string METADATA_BONDS = "Bonds";
const std::string METADATA_SIZE = "Size";
const std::string METADATA_BRICK_ID = "BrickId";

// Command line arguments
// - Database
const std::string ARG_DB_HOST = "--db-host";
const std::string ARG_DB_PORT = "--db-port";
const std::string ARG_DB_NAME = "--db-name";
const std::string ARG_DB_USER = "--db-user";
const std::string ARG_DB_PASSWORD = "--db-password";
const std::string ARG_DB_NB_CONNECTIONS = "--db-nb-connections";
const std::string ARG_DB_BATCH_SIZE = "--db-batch-size";
const size_t DEFAULT_DB_NB_CONNECTIONS = 8;
const size_t DEFAULT_BATCH_SIZE = 1000; // Max number of records returned by a batch query

// - Out-of-core
const std::string ARG_OOC_ENABLED = "--ooc-enabled";
const std::string ARG_OOC_VISIBLE_BRICKS = "--ooc-visible-bricks";
const std::string ARG_OOC_UPDATE_FREQUENCY = "--ooc-update-frequency";
const std::string ARG_OOC_UNLOAD_BRICKS = "--ooc-unload-bricks";
const std::string ARG_OOC_SHOW_GRID = "--ooc-show-grid";
const std::string ARG_OOC_NB_BRICKS_PER_CYCLE = "--ooc-nb-bricks-per-cycle";

// Environment variables
const std::string ENV_ROCKETS_DISABLE_SCENE_BROADCASTING = "ROCKETS_DISABLE_SCENE_BROADCASTING";

// Bezier curves parameters
const size_t DEFAULT_BEZIER_STEP = 4;

// Grid geometry alignment
const double NO_GRID_ALIGNMENT = 0.0;

namespace common
{
using MaterialSet = std::set<size_t>;
using Neighbours = std::set<size_t>;

class Node;
using NodePtr = std::shared_ptr<Node>;
using NodeMap = std::map<std::string, NodePtr>;

class Assembly;
using AssemblyPtr = std::shared_ptr<Assembly>;
using AssemblyMap = std::map<std::string, AssemblyPtr>;

enum class AssemblyConstraintType
{
    inside = 0,
    outside = 1
};
using AssemblyConstraint = std::pair<AssemblyConstraintType, AssemblyPtr>;
using AssemblyConstraints = std::vector<AssemblyConstraint>;

typedef struct
{
    core::Vector3d position;
    double radius;
    uint64_t sectionId{0};
    uint64_t graphId{0};
    uint64_t type{0};
    uint64_t pairId{0};
    uint64_t entryNodeId{0};
    uint64_t regionId{0};
} GeometryNode;
using GeometryNodes = std::map<uint64_t, GeometryNode>;
using GeometryEdges = std::map<uint64_t, uint64_t>;
using Bifurcations = std::map<uint64_t, uint64_ts>;

// Thread safe container
class ThreadSafeContainer;
using ThreadSafeContainers = std::vector<ThreadSafeContainer>;

// SDF structures
class SDFGeometries;
using SDFGeometriesPtr = std::shared_ptr<SDFGeometries>;

struct SDFMorphologyData
{
    std::vector<core::SDFGeometry> geometries;
    std::vector<std::set<size_t>> neighbours;
    size_ts materials;
    size_ts localToGlobalIdx;
    size_ts bifurcationIndices;
    std::unordered_map<size_t, int> geometrySection;
    std::unordered_map<int, size_ts> sectionGeometries;
};

enum class ReportType
{
    undefined = 0,
    spike = 1,
    soma = 2,
    compartment = 3,
    synapse_efficacy = 4
};

typedef struct
{
    ReportType type{ReportType::undefined};
    std::string description;
    double startTime;
    double endTime;
    double timeStep;
    std::string timeUnits;
    std::string dataUnits;
    bool debugMode{false};
    uint64_tm guids;
} SimulationReport;

/**
 * @brief File format for export of atom coordinates, radius and charge
 *
 */
enum class XYZFileFormat
{
    /** Unspecified */
    unspecified = 0,
    /** x, y, z coordinates stored in binary representation (4 byte double)
     */
    xyz_binary = 1,
    /** x, y, z coordinates and radius stored in binary representation (4
       byte double) */
    xyzr_binary = 2,
    /** x, y, z coordinates, radius, and charge stored in binary
       representation (4 byte double) */
    xyzrv_binary = 3,
    /** x, y, z coordinates stored in space separated ascii representation.
       One line per sphere*/
    xyz_ascii = 4,
    /** x, y, z coordinates and radius stored in space separated ascii
       representation. One line per sphere*/
    xyzr_ascii = 5,
    /** x, y, z coordinates, radius, and charge stored in space separated
       ascii representation. One line per sphere*/
    xyzrv_ascii = 6,
    /** x, y, z coordinates, radius, and RGBA color stored in space separated
       ascii representation. One line per sphere*/
    xyzr_rgba_ascii = 7
};
const std::string ASCII_FILE_SEPARATOR = ",";

typedef struct
{
    bool enabled{false};
    bool uniform{false};
    double radius{1.0};

} SpheresRepresentation;
} // namespace common

namespace molecularsystems
{
using ModelDescriptors = std::vector<core::ModelDescriptorPtr>;

class Membrane;
using MembranePtr = std::shared_ptr<Membrane>;

class Protein;
using ProteinPtr = std::shared_ptr<Protein>;
using ProteinMap = std::map<std::string, ProteinPtr>;
using Proteins = std::vector<ProteinPtr>;

class Glycans;
using GlycansPtr = std::shared_ptr<Glycans>;
using GlycansMap = std::map<std::string, GlycansPtr>;

class RNASequence;
using RNASequencePtr = std::shared_ptr<RNASequence>;
using RNASequenceMap = std::map<std::string, RNASequencePtr>;

class EnzymeReaction;
using EnzymeReactionPtr = std::shared_ptr<EnzymeReaction>;
using EnzymeReactionMap = std::map<std::string, EnzymeReactionPtr>;

/**
 * @brief Protein representation (atoms, atoms and sticks, etc)
 *
 */
enum class ProteinRepresentation
{
    /** Atoms only */
    atoms = 0,
    /** Atoms and sticks */
    atoms_and_sticks = 1,
    /** Protein contours */
    contour = 2,
    /** Protein surface computed using metaballs */
    surface = 3,
    /** Protein surface computed using union of balls */
    union_of_balls = 4,
    /** Debug mode, usually showing size and rotation of the protein */
    debug = 5,
    /** Precomputed OBJ meshes */
    mesh = 6
};

/**
 * @brief Structure containing information about an atom, as stored in a PDB
 * file
 *
 */
typedef struct
{
    /** Atom name */
    std::string name;
    /** Alternate location indicator */
    std::string altLoc;
    /** Residue name */
    std::string resName;
    /** Chain identifier */
    std::string chainId;
    /** Residue sequence number */
    size_t reqSeq;
    /** Code for insertions of residues */
    std::string iCode;
    /** orthogonal angstrom coordinates */
    core::Vector3d position;
    /** Occupancy */
    double occupancy;
    /** Temperature factor */
    double tempFactor;
    /** Element symbol */
    std::string element;
    /** Charge */
    std::string charge;
    /** Radius */
    double radius;
} Atom;
using AtomMap = std::multimap<size_t, Atom, std::less<size_t>>;

/**
 * @brief Sequence of residues
 *
 */
typedef struct
{
    /** Number of residues in the chain */
    size_t numRes;
    /** Residue name */
    strings resNames;
    /** Atom Offset */
    size_t offset;
} ResidueSequence;
using ResidueSequenceMap = std::map<std::string, ResidueSequence>;

/**
 * @brief Bonds map
 *
 */
using BondsMap = std::map<size_t, size_ts>;

/**
 * @brief Structure containing amino acids long and shot names
 *
 */
typedef struct
{
    /** Long name of the amino acid*/
    std::string name;
    /** Short name of the amino acid*/
    std::string shortName;
} AminoAcid;
using AminoAcidMap = std::map<std::string, AminoAcid>;

/**
 * @brief Set of residue names
 *
 */
using Residues = std::set<std::string>;

/**
 * @brief Atom radii in microns
 *
 */
using AtomicRadii = std::map<std::string, double>;

} // namespace molecularsystems

namespace atlas
{
class Atlas;
using AtlasPtr = std::shared_ptr<Atlas>;
} // namespace atlas

namespace vasculature
{
class Vasculature;
using VasculaturePtr = std::shared_ptr<Vasculature>;
} // namespace vasculature

namespace morphology
{
const std::string NEURON_CONFIG_MORPHOLOGY_FOLDER = "alternate_morphologies";
const std::string NEURON_CONFIG_MORPHOLOGY_FILE_EXTENSION = "morphology_file_extension";

class Morphologies;
using MorphologiesPtr = std::shared_ptr<Morphologies>;

class Astrocytes;
using AstrocytesPtr = std::shared_ptr<Astrocytes>;
class Neurons;
using NeuronsPtr = std::shared_ptr<Neurons>;
class Synapses;
using SynapsesPtr = std::shared_ptr<Synapses>;

enum class MorphologySynapseType
{
    none = 0,
    afferent = 1,
    efferent = 2,
    debug = 4,
    all = 8
};

typedef struct
{
    MorphologySynapseType type;
    uint64_t postSynapticNeuronId;
    uint64_t postSynapticSectionId;
    uint64_t postSynapticSegmentId;
    double preSynapticSegmentDistance;
    double postSynapticSegmentDistance;
    core::Vector3d preSynapticSurfacePosition;
    core::Vector3d postSynapticSurfacePosition;
} Synapse;
using SynapsesMap = std::map<uint64_t, Synapse>;
using SegmentSynapseMap = std::map<uint64_t, std::vector<Synapse>>;
using SectionSynapseMap = std::map<uint64_t, SegmentSynapseMap>;

typedef struct
{
    core::Vector3d center;
    double radius;
    uint64_ts children;
} AstrocyteSoma;
using AstrocyteSomaMap = std::map<uint64_t, AstrocyteSoma>;

typedef struct
{
    core::Vector3d position;
    core::Quaterniond rotation;
    uint64_t eType{0};
    uint64_t mType{0};
    uint64_t layer{0};
    uint64_t morphologyId{0};
} NeuronSoma;
using NeuronSomaMap = std::map<uint64_t, NeuronSoma>;

typedef struct
{
    core::Vector4fs points;
    size_t type;
    int64_t parentId;
    double length;
} Section;
using SectionMap = std::map<uint64_t, Section>;

typedef struct
{
    uint64_t vasculatureSectionId;
    uint64_t vasculatureSegmentId;
    double length;
    double radius;
    core::Vector4fs nodes;
} EndFoot;
using EndFootMap = std::map<uint64_t, EndFoot>;

typedef struct
{
    core::Vector3d position;
    core::Quaterniond rotation;
    uint64_t type{0};
    int64_t eType{0};
    uint64_t region{0};
} Cell;
using CellMap = std::map<uint64_t, Cell>;

const double mitochondrionSegmentSize = 0.25;
const double mitochondrionRadius = 0.1;

const double spineRadiusRatio = 0.5;

const double myelinSteathLength = 10.0;
const double myelinSteathRadiusRatio = 3.0;

const uint64_t nbMinSegmentsForVaricosity = 10;

enum class PopulationColorScheme
{
    /** All nodes use the same color */
    none = 0,
    /** Colored by id */
    id = 1
};

enum class MorphologyColorScheme
{
    /** All sections use the same color */
    none = 0,
    /** Colored by section */
    section_type = 1,
    /** section orientation */
    section_orientation = 2,
    /** distance to soma */
    distance_to_soma = 3
};

enum class MorphologyRepresentation
{
    graph = 0,
    section = 1,
    segment = 2,
    orientation = 3,
    bezier = 4,
    contour = 5,
    surface = 6,
    spheres = 7,
    uniform_spheres = 8
};

enum class MorphologyRealismLevel
{
    none = 0,
    soma = 1,
    axon = 2,
    dendrite = 4,
    internals = 8,
    externals = 16,
    spine = 32,
    end_foot = 64,
    all = 255,
};

enum class MicroDomainRepresentation
{
    mesh = 0,
    convex_hull = 1,
    surface = 2
};
using SpikesMap = std::map<uint64_t, float>;
} // namespace morphology

namespace connectomics
{
class WhiteMatter;
using WhiteMatterPtr = std::shared_ptr<WhiteMatter>;
using WhiteMatterStreamlines = std::vector<core::Vector3fs>;

class SynapseEfficacy;
using SynapseEfficacyPtr = std::shared_ptr<SynapseEfficacy>;

class Synaptome;
using SynaptomePtr = std::shared_ptr<Synaptome>;
} // namespace connectomics

namespace io
{
// Out of core brick manager
class OOCManager;
using OOCManagerPtr = std::shared_ptr<OOCManager>;

namespace filesystem
{
class MorphologyLoader;
using MorphologyLoaderPtr = std::shared_ptr<MorphologyLoader>;
} // namespace filesystem

namespace db
{
class DBConnector;
using DBConnectorPtr = std::shared_ptr<DBConnector>;

namespace fields
{
class FieldsHandler;
typedef std::shared_ptr<FieldsHandler> FieldsHandlerPtr;
} // namespace fields

} // namespace db
} // namespace io
namespace details
{
/**
 * @brief Structure defining the entry point response of the remote API
 *
 */
typedef struct
{
    /** Status of the response */
    bool status{true};
    /** Contents of the response (optional) */
    std::string contents;
} Response;

/**
 * @brief Structure defining on which instance of a model the camera should
 * focus on
 *
 */
typedef struct
{
    /** Model identifier */
    size_t modelId{0};
    /** Instance identifier */
    size_t instanceId{0};
    /** camera direction */
    doubles direction;
    /** Distance to the instance */
    double distance{0.0};
} FocusOnDetails;

/**
 * @brief Structure defining the plugin general settings
 *
 */
typedef struct
{
    std::string meshFolder;
    uint32_t loggingLevel;
    uint32_t databaseLoggingLevel;
    bool v1Compatibility{false};
    bool cacheEnabled{false};
    bool loadMorphologiesFromFileSystem{false};
} GeneralSettingsDetails;

typedef struct
{
    uint32_t seed{0};
    uint32_t positionSeed{0};
    double positionStrength{0.f};
    uint32_t rotationSeed{0};
    double rotationStrength{0.f};
    double morphingStep{0.f};
} MolecularSystemAnimationDetails;

/**
 * @brief Assembly shapes
 *
 */
enum class AssemblyShape
{
    /** Point */
    point = 0,
    /** Empty sphere */
    empty_sphere = 1,
    /** Planar */
    plane = 2,
    /** Sinusoidal */
    sinusoid = 3,
    /** Cubic */
    cube = 4,
    /** Fan */
    fan = 5,
    /** Bezier (experimental) */
    bezier = 6,
    /** mesh-based */
    mesh = 7,
    /** Helix */
    helix = 8,
    /** Filled sphere */
    filled_sphere = 9,
    /** Spherical cell diffusion */
    spherical_cell_diffusion = 10
};

/**
 * @brief Shapes that can be used to enroll RNA into the virus capsid
 *
 */
enum class RNAShapeType
{
    /** Trefoil knot */
    trefoilKnot = 0,
    /** Torus */
    torus = 1,
    /** Star */
    star = 2,
    /** Spring */
    spring = 3,
    /** Heart (all we need is love) */
    heart = 4,
    /** Thing (weird shape) */
    thing = 5,
    /** Moebius knot */
    moebius = 6
};

/**
 * @brief Object description in the 3D scene
 *
 */
typedef struct
{
    bool hit;
    std::string assemblyName;
    std::string proteinName;
    size_t modelId;
    size_t instanceId;
    doubles position;
} ProteinInspectionDetails;

typedef struct
{
    doubles origin;
    doubles direction;
} InspectionDetails;

/**
 * @brief Assembly representation
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string name;
    /** Shape of the assembly containing the parametric membrane */
    AssemblyShape shape;
    /** Shape parameters */
    doubles shapeParams;
    /** Contents of the mesh for mesh-based shapes */
    std::string shapeMeshContents;
    /** Position of the assembly in the scene */
    doubles position;
    /** rotation of the assembly in the scene */
    doubles rotation;
    /** Clipping planes applied to the loading of elements of the assembly */
    doubles clippingPlanes;
} AssemblyDetails;

/**
 * @brief Structure defining transformations to apply to assembly elements
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the element in the assembly to which transformations should be
     * applied */
    std::string name;
    /** List of transformations */
    doubles transformations;
} AssemblyTransformationsDetails;

/**
 * @brief A membrane is a shaped assembly of phospholipids
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the lipid in the assembly */
    std::string name;
    /** String containing a list of PDB ids for the lipids, delimited by
     * PDB_CONTENTS_DELIMITER */
    std::string lipidPDBIds;
    /** String containing a list of PDB description for the lipids, delimited by
     * PDB_CONTENTS_DELIMITER */
    std::string lipidContents;
    /** Relative rotation of the lipid in the membrane */
    doubles lipidRotation;
    /** Lipids density  */
    double lipidDensity;
    /** Multiplier applied to the radius of the lipid atoms */
    double atomRadiusMultiplier{1.0};
    /** Enable the loading of lipid bonds */
    bool loadBonds{false};
    /** Enable the loading of non polymer chemicals */
    bool loadNonPolymerChemicals{false};
    /** Defines the representation of the lipid (Atoms, atoms and sticks,
     * surface, etc) */
    molecularsystems::ProteinRepresentation representation{molecularsystems::ProteinRepresentation::atoms};
    /** Identifiers of chains to be loaded */
    size_ts chainIds;
    /** Recenters the lipid  */
    bool recenter{false};
    /** Extra optional parameters for positioning on the molecule */
    doubles animationParams;
} MembraneDetails;

// Protein
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the protein in the assembly */
    std::string name;
    /** PDB if of the protein */
    std::string pdbId;
    /** String containing a PDB representation of the protein */
    std::string contents;
    /** Multiplier applied to the radius of the protein atoms */
    double atomRadiusMultiplier{1.f};
    /** Enable the loading of protein bonds */
    bool loadBonds{false};
    /** Enable the loading of non polymer chemicals */
    bool loadNonPolymerChemicals{false};
    /** Enable the loading of hydrogen atoms */
    bool loadHydrogen{false};
    /** Defines the representation of the protein (Atoms, atoms and sticks,
     * surface, etc) */
    molecularsystems::ProteinRepresentation representation{molecularsystems::ProteinRepresentation::atoms};
    /** Identifiers of chains to be loaded */
    size_ts chainIds;
    /** Recenters the protein  */
    bool recenter{false};
    /** Number of protein occurrences to be added to the assembly */
    size_t occurrences{1};
    /** Indices of protein occurrences in the assembly for which proteins are
     * added */
    size_ts allowedOccurrences;
    /** Trans-membrane parameters */
    doubles transmembraneParams;
    /** Extra optional parameters for positioning on the molecule */
    doubles animationParams;
    /** Relative position of the protein in the assembly */
    doubles position;
    /** Relative rotation of the protein in the assembly */
    doubles rotation;
    /** List of assembly names used to constrain the placement of the protein.
     * If the assembly name is prefixed by a +, proteins are not allowed inside
     * the spedified assembly. If the name is prefixed by a -, proteins are not
     * allowed outside of the assembly */
    std::string constraints;
} ProteinDetails;

/**
 * @brief Data structure describing the sugar
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the sugar in the assembly */
    std::string name;
    /** String containing the PDB Id of the sugar */
    std::string pdbId;
    /** String containing a PDB representation of the sugar */
    std::string contents;
    /** Name of the protein on which sugar are added */
    std::string proteinName;
    /** Multiplier applied to the radius of the molecule atoms */
    double atomRadiusMultiplier{1.0};
    /** Enable the loading of molecule bonds */
    bool loadBonds{false};
    /** Defines the representation of the molecule (Atoms, atoms and sticks,
     * surface, etc) */
    molecularsystems::ProteinRepresentation representation{molecularsystems::ProteinRepresentation::atoms};
    /** Recenters the protein  */
    bool recenter{true};
    /** Identifiers of chains to be loaded */
    size_ts chainIds;
    /** List of sites on which sugar can be added */
    size_ts siteIndices;
    /** Relative rotation of the sugar on the molecule */
    doubles rotation;
    /** Extra optional parameters for positioning on the molecule */
    doubles animationParams;
} SugarDetails;

/**
 * @brief RNA sequence descriptor
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the RNA sequence in the assembly */
    std::string name;
    /** String containing the PDB id of the N protein */
    std::string pdbId;
    /** A string containing the list of codons */
    std::string contents;
    /** A string containing an PDB representation of the N protein */
    std::string proteinContents;
    /** A given shape */
    RNAShapeType shape;
    /** Shape radius */
    doubles shapeParams;
    /** Range of values used to compute the shape */
    doubles valuesRange;
    /** Parameters used to compute the shape */
    doubles curveParams;
    /** Multiplier applied to the radius of the molecule atoms */
    double atomRadiusMultiplier{1.0};
    /** Defines the representation of the molecule (Atoms, atoms and sticks,
     * surface, etc) */
    molecularsystems::ProteinRepresentation representation{molecularsystems::ProteinRepresentation::atoms};
    /** Animation params */
    doubles animationParams;
    /** Relative position of the RNA sequence in the assembly */
    doubles position;
    /** Relative rotation of the RNA sequence in the assembly */
    doubles rotation;
} RNASequenceDetails;

/**
 * @brief Structure defining a selection of amino acids on a protein of
 * an assembly. The selection is defined as a string
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the protein in the assembly */
    std::string name;
    /** String containing the amino acid sequence to select */
    std::string sequence;
} AminoAcidSequenceAsStringDetails;

/**
 * @brief Structure defining a selection of amino acids on a protein of an
 * assembly. The selection is defined as a range of indices
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the protein in the assembly */
    std::string name;
    /** List of tuples of 2 integers defining indices in the sequence of
     * amino acid */
    size_ts ranges;
} AminoAcidSequenceAsRangesDetails;

typedef struct
{
    std::string assemblyName;
    std::string name;
} AminoAcidInformationDetails;

/**
 * @brief Structure used to set an amino acid in protein sequences
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the protein */
    std::string name;
    /** Index of the amino acid in the sequence */
    size_t index;
    /** Amino acid short name */
    std::string aminoAcidShortName;
    /** List of chains in which the amino acid is set */
    size_ts chainIds;
} AminoAcidDetails;

/**
 * @brief An enzyme reaction
 *
 */
typedef struct
{
    /** Name of the assembly that owns the enzyme reaction */
    std::string assemblyName;
    /** Name of the enzyme reaction in the assembly */
    std::string name;
    /** String containing a list of PDB description for the enzyme protein */
    std::string enzymeName;
    /** String containing a list substrate names */
    std::string substrateNames;
    /** String containing a list of product names */
    std::string productNames;
} EnzymeReactionDetails;

/**
 * @brief Progress of an enzyme reaction for a given instance
 *
 */
typedef struct
{
    /** Name of the assembly that owns the enzyme reaction */
    std::string assemblyName;
    /** Name of the enzyme reaction in the assembly */
    std::string name;
    /** Instance of the substrate molecule */
    size_t instanceId{0};
    /** Double containing the progress of the reaction (0..1) */
    double progress{0.0};
} EnzymeReactionProgressDetails;

/**
 * @brief Defines the parameters needed when adding 3D grid in the scene
 *
 */
typedef struct
{
    /** Minimum value on the axis */
    double minValue{0.0};
    /** Maximum value on the axis */
    double maxValue{100.0};
    /** Interval between lines of the grid */
    double steps{10.0};
    /** Radius of the lines */
    double radius{1.0};
    /** Opacity of the grid */
    double planeOpacity{1.0};
    /** Defines if axes should be shown */
    bool showAxis{true};
    /** Defines if planes should be shown */
    bool showPlanes{true};
    /** Defines if full grid should be shown */
    bool showFullGrid{false};
    /** Defines if the RGB color scheme shoudl be applied to axis */
    bool useColors{true};
    /** Position of the grid in the scene */
    doubles position;
} AddGridDetails;

/**
 * @brief Defines the parameters needed when adding sphere to the scene
 *
 */
typedef struct
{
    /** Name of the spheres */
    std::string name;
    /** Positions of the spheres in the scene */
    doubles positions;
    /** Radii of the sphere */
    doubles radii;
    /** RGB Color of the sphere */
    doubles color{1.0, 1.0, 1.0};
    /** Opacity */
    double opacity{1.0};
} AddSpheresDetails;

/**
 * @brief Defines the parameters needed when adding cone to the scene
 *
 */
typedef struct
{
    /** Name of the cones */
    std::string name;
    /** Origin of the cone in the scene */
    doubles origins;
    /** Target of the cone in the scene */
    doubles targets;
    /** Origin radii of the cones */
    doubles originsRadii;
    /** Target radii of the cones */
    doubles targetsRadii;
    /** RGB Color of the cone */
    doubles color{1.0, 1.0, 1.0};
    /** Opacity */
    double opacity;
} AddConesDetails;

/**
 * @brief Defines the parameters needed when adding bounding box to the scene
 */
typedef struct
{
    /** Name of the bounding box */
    std::string name;
    /** Position of the bottom left corner in the scene */
    doubles bottomLeft;
    /** Position of the top right corner in the scene */
    doubles topRight;
    /** Radius of the borders */
    double radius{1.0};
    /** RGB Color of the bounding box */
    doubles color{1.0, 1.0, 1.0};
} AddBoundingBoxDetails;

/**
 * @brief Defines the parameters needed when adding box to the scene
 */
typedef struct
{
    /** Name of the box */
    std::string name;
    /** Position of the bottom left corner in the scene */
    doubles bottomLeft;
    /** Position of the top right corner in the scene */
    doubles topRight;
    /** RGB Color of the box */
    doubles color;
} AddBoxDetails;

/**
 * @brief The Streamlines struct handles a set of streamlines. Indices are used
 * to specify the first point of each streamline
 */
typedef struct
{
    /** Name of the streamlines */
    std::string name;
    /** Indices */
    uint64_ts indices;
    /** Vertices */
    doubles vertices;
    /** Vertices */
    doubles colors;
} AddStreamlinesDetails;

/**
 * @brief Color schemes that can be applied to proteins
 *
 */
enum class ProteinColorScheme
{
    /** All atoms use the same color */
    none = 0,
    /** Colored by atom according to the Pymol color scheme */
    atoms = 1,
    /** Colored by chain */
    chains = 2,
    /** Colored by residue */
    residues = 3,
    /** Colored by sequence of amino acids */
    amino_acid_sequence = 4,
    /** Colored by glysolysation site */
    glycosylation_site = 5,
    /** Colored by functional region */
    region = 6
};

/**
 * @brief Defines the color scheme to apply to a protein
 *
 */
typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the protein in the assembly */
    std::string name;
    /** Color scheme **/
    ProteinColorScheme colorScheme;
    /** Palette of colors (RGB values) */
    doubles palette;
    /** Ids of protein chains to which the colors scheme is applied */
    size_ts chainIds;
} ProteinColorSchemeDetails;

typedef struct
{
    /** Name of the assembly */
    std::string assemblyName;
    /** Name of the protein in the assembly */
    std::string name;
    /** Index of the protein instance */
    size_t instanceIndex;
    /** Position of the protein instance */
    doubles position;
    /** rotation of the protein instance */
    doubles rotation;
} ProteinInstanceTransformationDetails;

/**
 * @brief List of identifiers
 *
 */
typedef struct
{
    /** List of identifiers */
    size_ts ids;
} IdsDetails;

/**
 * @brief Model name
 *
 */
typedef struct
{
    /** Element name */
    std::string name;
} NameDetails;

/**
 * @brief Model identifier
 *
 */
typedef struct
{
    /** Model identifier */
    size_t modelId;
    /** Maximum number of instances that can be processed */
    size_t maxNbInstances;
} ModelIdDetails;

/**
 * @brief Model transformation
 *
 */
typedef struct
{
    /** Translation */
    doubles translation;
    /** Rotation */
    doubles rotation;
    /** Rotation center */
    doubles rotationCenter;
    /** Scale */
    doubles scale;
} ModelTransformationDetails;

/**
 * @brief Add instance to model
 *
 */
typedef struct
{
    /** Model identifier */
    size_t modelId;
    /** Translation */
    doubles translation;
    /** Rotation */
    doubles rotation;
    /** Rotation center */
    doubles rotationCenter;
    /** Scale */
    doubles scale;
} AddModelInstanceDetails;

/**
 * @brief Set instances to model
 *
 */
typedef struct
{
    /** Model identifier */
    size_t modelId;
    /** Translation */
    doubles translations;
    /** Rotation */
    doubles rotations;
    /** Rotation center */
    doubles rotationCenters;
    /** Scale */
    doubles scales;
} SetModelInstancesDetails;

/**
 * @brief Model identifier
 *
 */
typedef struct
{
    /** Model min bounding box coordinates */
    doubles minAABB;
    /** Model max bounding box coordinates */
    doubles maxAABB;
    /** Model bounding box center */
    doubles center;
    /** Model bounding box size */
    doubles size;
} ModelBoundsDetails;

/**
 * @brief Structure containing attributes of materials attached to one or
 several Core models
 */
typedef struct
{
    /** List of model identifiers */
    int32_ts modelIds;
    /** List of material identifiers */
    int64_ts materialIds;
    /** List of RGB values for diffuse colors */
    doubles diffuseColors;
    /** List of RGB values for specular colors */
    doubles specularColors;
    /** List of values for specular exponents */
    doubles specularExponents;
    /** List of values for reflection indices */
    doubles reflectionIndices;
    /** List of values for opacities */
    doubles opacities;
    /** List of values for refraction indices */
    doubles refractionIndices;
    /** List of values for light emission */
    doubles emissions;
    /** List of values for glossiness */
    doubles glossinesses;
    /** List of values for casting user data */
    bools castUserData;
    /** List of values for shading modes */
    int32_ts shadingModes;
    /** List of values for user defined parameters */
    doubles userParameters;
    /** List of values for chameleon mode parameters */
    int32_ts chameleonModes;
    /** List of values for clipping mode parameters */
    int32_ts clippingModes;
} MaterialsDetails;

/** Field data type */
enum class FieldDataType
{
    /** Point field (spheres) */
    fdt_points = 0,
    /** Vector field */
    fdt_vectors = 1
};

/**
 * @brief Structure containing information about how to build magnetic
 * fields from atom positions and charge
 *
 */
typedef struct
{
    /** Voxel size used to build the Octree acceleration structure */
    double voxelSize;
    /** Density of atoms to consider (Between 0 and 1) */
    double density;
    /** Field type*/
    FieldDataType dataType;
    /** Model ids*/
    uint32_ts modelIds;
} BuildFieldsDetails;

// IO
typedef struct
{
    size_t modelId;
    std::string filename;
} ModelIdFileAccessDetails;

/**
 * @brief Structure defining how to export data into a file
 *
 */
typedef struct
{
    std::string filename;
    doubles lowBounds;
    doubles highBounds;
    common::XYZFileFormat fileFormat;
} FileAccessDetails;

/**
 * @brief Structure defining how to export data into a LAS file
 *
 */
typedef struct
{
    std::string filename;
    uint32_ts modelIds;
    uint32_ts materialIds;
    bool exportColors{false};
} LASFileAccessDetails;

/**
 * @brief Structure defining how to export data into a DB
 *
 */
typedef struct
{
    int32_t brickId;
    doubles lowBounds;
    doubles highBounds;
} DatabaseAccessDetails;

/**
 * @brief Structure defining how to build a point cloud from the scene
 *
 */
typedef struct
{
    double radius;
} BuildPointCloudDetails;

enum class ModelLoadingTransactionAction
{
    start = 0,
    commit = 1
};

/**
 * @brief Structure defining how visible models are in the scene
 *
 */
typedef struct
{
    ModelLoadingTransactionAction action;
} ModelLoadingTransactionDetails;

typedef struct
{
    /** Description of the scene **/
    std::string description;
    /** Size of the scene */
    core::Vector3d sceneSize;
    /** Number of bricks per side of the scene */
    uint32_t nbBricks;
    /** Size of the each brick in the scene */
    core::Vector3d brickSize;
} OOCSceneConfigurationDetails;

/**
 * @brief List of metrics for the current scene
 *
 */
typedef struct
{
    /** Number of models */
    uint32_t nbModels{0};
    /** Number of materials */
    uint32_t nbMaterials{0};
    /** Number of spheres */
    uint32_t nbSpheres{0};
    /** Number of cylinders */
    uint32_t nbCylinders{0};
    /** Number of cones */
    uint32_t nbCones{0};
    /** Number of triangle mesh vertices */
    uint32_t nbVertices{0};
    /** Number of triangle mesh indices */
    uint32_t nbIndices{0};
    /** Number of triangle mesh normals */
    uint32_t nbNormals{0};
    /** Number of triangle mesh colors */
    uint32_t nbColors{0};
} SceneInformationDetails;

/**
 * @brief Brain atlas
 *
 */
typedef struct
{
    /** Name of the assembly containing the atlas */
    std::string assemblyName;
    /** Population name */
    std::string populationName;
    /** Load cells if set to true */
    bool loadCells{true};
    /** Cell radius **/
    double cellRadius{1.f};
    /** Load region meshes if set to true */
    bool loadMeshes{true};
    /** SQL filter for cells (WHERE condition) */
    std::string cellSqlFilter;
    /** SQL filter for regions (WHERE condition) */
    std::string regionSqlFilter;
    /** Scale of the atlas in the scene */
    doubles scale{1.0, 1.0, 1.0};
    /** Mesh transformation */
    doubles meshPosition;
    doubles meshRotation;
    doubles meshScale{1.0, 1.0, 1.0};
} AtlasDetails;

// -------------------------------------------------------------------------------------------------
// Vasculature
// -------------------------------------------------------------------------------------------------
enum class VasculatureRealismLevel
{
    none = 0,
    section = 1,
    bifurcation = 2,
    all = 255,
};

/**
 * @brief Color schemes that can be applied to vasculatures
 *
 */
enum class VasculatureColorScheme
{
    /** All edges use the same color */
    none = 0,
    /** Colored by node */
    node = 1,
    /** Colored by section */
    section = 2,
    /** Colored by sub-graph */
    subgraph = 3,
    /** Colored by pair */
    pair = 4,
    /** Colored by entry node */
    entry_node = 5,
    /** Colored by radius */
    radius = 6,
    /** Colored by point order within a section */
    section_points = 7,
    /** Colored by section orientation */
    section_orientation = 8,
    /** Colored by region */
    region = 9
};

enum class VasculatureRepresentation
{
    graph = 0,
    section = 1,
    segment = 2,
    optimized_segment = 3,
    bezier = 4,
    spheres = 5,
    uniform_spheres = 6
};

typedef struct
{
    /** Name of the assembly containing the vasculature */
    std::string assemblyName;
    /** Population name */
    std::string populationName;
    /** Color scheme **/
    VasculatureColorScheme colorScheme{VasculatureColorScheme::none};
    /** Use Signed Distance Fields for geometry realism */
    int64_t realismLevel{0};
    /** Node gids to load. All if empty */
    uint32_ts gids;
    /** Geometry quality */
    VasculatureRepresentation representation{VasculatureRepresentation::segment};
    /** Multiplies the vasculature section radii by the specified value */
    double radiusMultiplier{1.0};
    /** SQL filter (WHERE condition) */
    std::string sqlFilter;
    /** Scale of the vasculature in the scene */
    doubles scale{1.0, 1.0, 1.0};
    /** Extra optional parameters for neuron animation */
    doubles animationParams;
    /** Extra optional parameters for geometry displacement */
    doubles displacementParams;
    /** Align 3D positions to grid if different from 0.0 */
    double alignToGrid{0.0};
} VasculatureDetails;

typedef struct
{
    /** Name of the assembly containing the vasculature */
    std::string assemblyName;
    /** Name of the population on which the report applies */
    std::string populationName;
    /** Simulation report ID */
    uint64_t simulationReportId;
    /** Show evolution  */
    bool showEvolution;
} VasculatureReportDetails;

typedef struct
{
    /** Name of the assembly containing the vasculature */
    std::string assemblyName;
    /** Name of the population on which the report applies */
    std::string populationName;
    /** Simulation report ID */
    uint64_t simulationReportId{0};
    /** Simulation frame number */
    uint64_t frame{0};
    /** Amplitude applied to the radius */
    double amplitude{1.0};
} VasculatureRadiusReportDetails;

typedef struct
{
    /** Name of the assembly containing the astrocytes */
    std::string assemblyName;
    /** Name of the population of astrocytes */
    std::string populationName;
    /** Name of the vasculature population. If not empty, endfeet are automatically loaded */
    std::string vasculaturePopulationName;
    /** Name of the connectome population. If not empty, endfeet are automatically loaded */
    std::string connectomePopulationName;
    /** Load somas if set to true */
    bool loadSomas{true};
    /** Load dendrites if set to true */
    bool loadDendrites{true};
    /** Generate internal components (nucleus and mitochondria) */
    bool generateInternals{false};
    /** Load micro-domain */
    bool loadMicroDomains{false};
    /** Use Signed Distance Fields for geometry realism */
    int64_t realismLevel{0};
    /** Morphology representation */
    morphology::MorphologyRepresentation morphologyRepresentation{morphology::MorphologyRepresentation::segment};
    /** Micro-domain representation */
    morphology::MicroDomainRepresentation microDomainRepresentation{morphology::MicroDomainRepresentation::mesh};
    /** Geometry color scheme */
    morphology::MorphologyColorScheme morphologyColorScheme{morphology::MorphologyColorScheme::none};
    /** Population color scheme */
    morphology::PopulationColorScheme populationColorScheme{morphology::PopulationColorScheme::none};
    /** Multiplies the astrocyte section radii by the specified value */
    double radiusMultiplier{1.0};
    /** SQL filter (WHERE condition) */
    std::string sqlFilter;
    /** Scale of the astrocyte in the scene */
    doubles scale{1.0, 1.0, 1.0};
    /** Extra optional parameters for astrocytes animation */
    doubles animationParams;
    /** Extra optional parameters for geometry displacement */
    doubles displacementParams;
    /** Only load segments that with distance to soma smaller than specified
     * value. Ignored if set to 0 */
    double maxDistanceToSoma{0.0};
    /** Align 3D positions to grid if different from 0.0 */
    double alignToGrid{0.0};
} AstrocytesDetails;

enum class NeuronSectionType
{
    undefined = 0,
    soma = 1,
    axon = 2,
    basal_dendrite = 3,
    apical_dendrite = 4
};
using NeuronSectionTypes = std::vector<NeuronSectionType>;
const int64_t SOMA_AS_PARENT = -1;

typedef struct
{
    int64_t reportId{-1};
    core::Vector2d valueRange{-100, 100};
    core::Vector2d voltageScalingRange{0.1, 1.0};
    uint64_t initialSimulationFrame{0};
    bool loadNonSimulatedNodes{false};
    bool voltageScalingEnabled{false};
} NeuronsReportParameters;

typedef struct
{
    /** Name of the assembly containing the astrocytes */
    std::string assemblyName;
    /** Name of the population of astrocytes */
    std::string populationName;
    /** Load somas if set to true */
    bool loadSomas{true};
    /** Load axons if set to true */
    bool loadAxon{true};
    /** Load bascal dendrites if set to true */
    bool loadBasalDendrites{true};
    /** Load apical dendrites if set to true */
    bool loadApicalDendrites{true};
    /** Type of synapses to load */
    morphology::MorphologySynapseType synapsesType{morphology::MorphologySynapseType::none};
    /** Generate internal components (nucleus and mitochondria) */
    bool generateInternals{false};
    /** Generate external components (myelin steath) */
    bool generateExternals{false};
    /** Show membrane (Typically used to isolate internal and external
     * components*/
    bool showMembrane{true};
    /** Generates random varicosities along the axon */
    bool generateVaricosities{false};
    /** Use Signed Distance Fields for geometry realism */
    int64_t realismLevel{0};
    /** Morphology representation */
    morphology::MorphologyRepresentation morphologyRepresentation{morphology::MorphologyRepresentation::segment};
    /** Geometry color scheme */
    morphology::MorphologyColorScheme morphologyColorScheme{morphology::MorphologyColorScheme::none};
    /** Population color scheme */
    morphology::PopulationColorScheme populationColorScheme{morphology::PopulationColorScheme::none};
    /** Multiplies the astrocyte section radii by the specified value */
    double radiusMultiplier{1.0};
    /** Report parameters */
    doubles reportParams;
    /** SQL filter for nodes (WHERE condition) */
    std::string sqlNodeFilter;
    /** SQL filter dor sections (WHERE condition) */
    std::string sqlSectionFilter;
    /** Scale of the neuron in the scene */
    doubles scale{1.0, 1.0, 1.0};
    /** Extra optional parameters for neuron animation */
    doubles animationParams;
    /** Extra optional parameters for geometry displacement */
    doubles displacementParams;
    /** Only load segments that with distance to soma smaller than specified
     * value. Ignored if set to 0 */
    double maxDistanceToSoma{0.0};
    /** Align 3D positions to grid if different from 0.0 */
    double alignToGrid{0.0};
} NeuronsDetails;

typedef struct
{
    /** Name of the assembly containing the neurons */
    std::string assemblyName;
    /** Neuron identifier */
    uint64_t neuronId{0};
    /** Section identifier */
    uint64_t sectionId{0};
} NeuronIdSectionIdDetails;

typedef struct
{
    /** Name of the assembly containing the neurons */
    std::string assemblyName;
    /** Neuron identifier */
    uint64_t neuronId{0};
} NeuronIdDetails;

typedef struct
{
    bool status{true};
    doubles points;
} NeuronPointsDetails;

typedef struct
{
    doubles source;
    doubles target;
} LookAtDetails;

typedef struct
{
    doubles rotation;
} LookAtResponseDetails;

typedef struct
{
    /** Name of the assembly containing the white matter */
    std::string assemblyName;
    /** Name of the white matter population  */
    std::string populationName;
    /** Streamline radius */
    double radius{1.0};
    /** SQL filter for streamlines (WHERE condition) */
    std::string sqlFilter;
    /** Scale of the streamlines in the scene */
    doubles scale{1.0, 1.0, 1.0};
} WhiteMatterDetails;

typedef struct
{
    /** Name of the assembly containing the graph */
    std::string assemblyName;
    /** Name of the white matter population  */
    std::string populationName;
    /** radius */
    double radius{1.0};
    /** radius */
    double force{1.0};
    /** SQL filter for neurons (WHERE condition) */
    std::string sqlNodeFilter;
    /** SQL filter for synapses (WHERE condition) */
    std::string sqlEdgeFilter;
} SynaptomeDetails;

enum class SynapseRepresentation
{
    sphere = 0,
    spine = 1
};

typedef struct
{
    /** Name of the assembly containing the white matter */
    std::string assemblyName;
    /** Name of the white matter population  */
    std::string populationName;
    /** Multiplies the spine  radii by the specified value */
    double radiusMultiplier{1.0};
    /** Representation of the synapse (sphere or spine) */
    SynapseRepresentation representation{SynapseRepresentation::sphere};
    /** Use Signed Distance Fields for geometry realism */
    int64_t realismLevel{0};
    /** SQL filter for streamlines (WHERE condition) */
    std::string sqlFilter;
    /** Extra optional parameters for geometry displacement */
    doubles displacementParams;
} SynapsesDetails;

typedef struct
{
    /** Name of the assembly containing the white matter */
    std::string assemblyName;
    /** Name of the white matter population  */
    std::string populationName;
    /** Streamline radius */
    double radius{1.0};
    /** SQL filter for streamlines (WHERE condition) */
    std::string sqlFilter;
    /** Simulation report identifier */
    int64_t simulationReportId{-1};
    /** Align 3D positions to grid if different from 0.0 */
    double alignToGrid{0.0};
} SynapseEfficacyDetails;

typedef struct
{
    uint64_t modelId;
    float restVoltage;
    float spikingVoltage;
    float timeInterval;
    float decaySpeed;
} SpikeReportVisualizationSettingsDetails;

typedef struct
{
    uint32_t seed{0};
    uint32_t offset{0};
    double amplitude{1.0};
    double frequency{1.0};
} CellAnimationDetails;

typedef struct
{
    std::string name;
    doubles position;
    double outerRadius;
    double innerRadius;
    doubles displacement;
} SDFTorusDetails;

typedef struct
{
    std::string name;
    doubles srcPosition;
    doubles dstPosition;
    double radius;
    doubles displacement;
} SDFVesicaDetails;

typedef struct
{
    std::string name;
    doubles position;
    doubles radii;
    doubles displacement;
} SDFEllipsoidDetails;
} // namespace details
} // namespace bioexplorer

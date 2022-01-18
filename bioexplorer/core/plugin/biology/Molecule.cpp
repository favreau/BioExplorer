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

#include "Molecule.h"

#include <plugin/common/CommonTypes.h>
#include <plugin/common/GeneralSettings.h>
#include <plugin/common/Logs.h>
#include <plugin/common/Utils.h>

#ifdef USE_CGAL
#include <plugin/meshing/PointCloudMesher.h>
#include <plugin/meshing/SurfaceMesher.h>
#endif

#include <brayns/engineapi/Material.h>

#include <omp.h>

namespace bioexplorer
{
namespace biology
{
using namespace common;
#ifdef USE_CGAL
using namespace meshing;
#endif

const std::string METADATA_AA_RANGE = "Amino acids range";
const std::string METADATA_AA_SEQUENCE = "Amino Acid Sequence";

// Atomic radii in picometers (10e-12 meters)
const double DEFAULT_ATOM_RADIUS = 25.0;
static AtomicRadii atomicRadii = {{{"H"}, {53.0}},
                                  {{"C"}, {67.0}},
                                  {{"N"}, {56.0}},
                                  {{"O"}, {48.0}},
                                  {{"H"}, {53.0}},
                                  {{"B"}, {87.0}},
                                  {{"F"}, {42.0}},
                                  {{"P"}, {98.0}},
                                  {{"S"}, {88.0}},
                                  {{"V"}, {171.0}},
                                  {{"K"}, {243.0}},
                                  {{"HE"}, {31.0}},
                                  {{"LI"}, {167.0}},
                                  {{"BE"}, {112.0}},
                                  {{"NE"}, {38.0}},
                                  {{"NA"}, {190.0}},
                                  {{"MG"}, {145.0}},
                                  {{"AL"}, {118.0}},
                                  {{"SI"}, {111.0}},
                                  {{"CL"}, {79.0}},
                                  {{"Cl"}, {79.0}},
                                  {{"AR"}, {71.0}},
                                  {{"CA"}, {194.0}},
                                  {{"SC"}, {184.0}},
                                  {{"TI"}, {176.0}},
                                  {{"CR"}, {166.0}},
                                  {{"MN"}, {161.0}},
                                  {{"FE"}, {156.0}},
                                  {{"CO"}, {152.0}},
                                  {{"NI"}, {149.0}},
                                  {{"CU"}, {145.0}},
                                  {{"ZN"}, {142.0}},
                                  {{"GA"}, {136.0}},
                                  {{"GE"}, {125.0}},
                                  {{"AS"}, {114.0}},
                                  {{"SE"}, {103.0}},
                                  {{"BR"}, {94.0}},
                                  {{"KR"}, {88.0}},
                                  // TODO
                                  {{"OD1"}, {25.0}},
                                  {{"OD2"}, {25.0}},
                                  {{"CG1"}, {25.0}},
                                  {{"CG2"}, {25.0}},
                                  {{"CD1"}, {25.0}},
                                  {{"CB"}, {25.0}},
                                  {{"CG"}, {25.0}},
                                  {{"CD"}, {25.0}},
                                  {{"OE1"}, {25.0}},
                                  {{"NE2"}, {25.0}},
                                  {{"CZ"}, {25.0}},
                                  {{"NH1"}, {25.0}},
                                  {{"NH2"}, {25.0}},
                                  {{"CD2"}, {25.0}},
                                  {{"CE1"}, {25.0}},
                                  {{"CE2"}, {25.0}},
                                  {{"CE"}, {25.0}},
                                  {{"NZ"}, {25.0}},
                                  {{"OH"}, {25.0}},
                                  {{"CE"}, {25.0}},
                                  {{"ND1"}, {25.0}},
                                  {{"ND2"}, {25.0}},
                                  {{"OXT"}, {25.0}},
                                  {{"OG1"}, {25.0}},
                                  {{"NE1"}, {25.0}},
                                  {{"CE3"}, {25.0}},
                                  {{"CZ2"}, {25.0}},
                                  {{"CZ3"}, {25.0}},
                                  {{"CH2"}, {25.0}},
                                  {{"OE2"}, {25.0}},
                                  {{"OG"}, {25.0}},
                                  {{"OE2"}, {25.0}},
                                  {{"SD"}, {25.0}},
                                  {{"SG"}, {25.0}},
                                  {{"C1*"}, {25.0}},
                                  {{"C2"}, {25.0}},
                                  {{"C2*"}, {25.0}},
                                  {{"C3*"}, {25.0}},
                                  {{"C4"}, {25.0}},
                                  {{"C4*"}, {25.0}},
                                  {{"C5"}, {25.0}},
                                  {{"C5*"}, {25.0}},
                                  {{"C5M"}, {25.0}},
                                  {{"C6"}, {25.0}},
                                  {{"C8"}, {25.0}},
                                  {{"H1"}, {25.0}},
                                  {{"H1*"}, {25.0}},
                                  {{"H2"}, {25.0}},
                                  {{"H2*"}, {25.0}},
                                  {{"H3"}, {25.0}},
                                  {{"H3*"}, {25.0}},
                                  {{"H3P"}, {25.0}},
                                  {{"H4"}, {25.0}},
                                  {{"H4*"}, {25.0}},
                                  {{"H5"}, {25.0}},
                                  {{"H5*"}, {25.0}},
                                  {{"H5M"}, {25.0}},
                                  {{"H6"}, {25.0}},
                                  {{"H8"}, {25.0}},
                                  {{"N1"}, {25.0}},
                                  {{"N2"}, {25.0}},
                                  {{"N3"}, {25.0}},
                                  {{"N4"}, {25.0}},
                                  {{"N6"}, {25.0}},
                                  {{"N7"}, {25.0}},
                                  {{"N9"}, {25.0}},
                                  {{"O1P"}, {25.0}},
                                  {{"O2"}, {25.0}},
                                  {{"O2P"}, {25.0}},
                                  {{"O3*"}, {25.0}},
                                  {{"O3P"}, {25.0}},
                                  {{"O4"}, {25.0}},
                                  {{"O4*"}, {25.0}},
                                  {{"O5*"}, {25.0}},
                                  {{"O6"}, {25.0}},
                                  {{"OXT"}, {25.0}},
                                  {{"P"}, 25.0}};

Molecule::Molecule(Scene& scene, const size_ts& chainIds)
    : _aminoAcidRange(std::numeric_limits<size_t>::max(),
                      std::numeric_limits<size_t>::min())
    , _scene(scene)
    , _chainIds(chainIds)
{
}

void Molecule::_computeReqSetOffset()
{
    for (auto& sequence : _residueSequenceMap)
    {
        std::string physicalReqSeq;
        size_t firstReqSeq;
        size_t previousReqSeq;
        bool initialized{false};
        for (const auto& atom : _atomMap)
        {
            if (atom.second.chainId != sequence.first)
                continue;

            if (!initialized)
            {
                firstReqSeq = atom.second.reqSeq;
                previousReqSeq = firstReqSeq - 1;
                initialized = true;
            }

            if (previousReqSeq != atom.second.reqSeq)
            {
                if (atom.second.reqSeq != previousReqSeq + 1)
                    break;

                const auto it = aminoAcidMap.find(atom.second.resName);
                if (it != aminoAcidMap.end())
                    physicalReqSeq += it->second.shortName;

                if (physicalReqSeq.length() > 10)
                    break;
            }

            previousReqSeq = atom.second.reqSeq;
        }

        std::string theoreticalReqSeq;
        for (const auto& aa : sequence.second.resNames)
            theoreticalReqSeq += aminoAcidMap.find(aa)->second.shortName;

        sequence.second.offset =
            theoreticalReqSeq.find(physicalReqSeq) - firstReqSeq;
        PLUGIN_INFO(3, "Sequence [" << sequence.first
                                    << "], offset: " << sequence.second.offset
                                    << ", Theoretical: " << theoreticalReqSeq
                                    << ", Physical: " << physicalReqSeq);
    }
}

const StringMap Molecule::getSequencesAsString() const
{
    StringMap sequencesAsStrings;
    for (const auto& sequence : _residueSequenceMap)
    {
        std::string shortSequence = std::to_string(_aminoAcidRange.x) + "," +
                                    std::to_string(_aminoAcidRange.y) + ",";
        for (const auto& resName : sequence.second.resNames)
            shortSequence += aminoAcidMap[resName].shortName;

        sequencesAsStrings[sequence.first] = shortSequence;
        PLUGIN_DEBUG(sequence.first << " (" << sequence.second.resNames.size()
                                    << "): " << shortSequence);
    }
    return sequencesAsStrings;
}

void Molecule::_buildAtomicStruture(const ProteinRepresentation representation,
                                    const double atomRadiusMultiplier,
                                    const bool surface, const bool loadBonds,
                                    Model& model)
{
    // Atoms
    for (const auto& atom : _atomMap)
    {
        // Material
        auto material =
            model.createMaterial(atom.first, std::to_string(atom.first));

        RGBColorDetails rgb{255, 255, 255};
        const auto it = atomColorMap.find(atom.second.element);
        if (it != atomColorMap.end())
            rgb = (*it).second;

        brayns::PropertyMap props;
        props.setProperty({MATERIAL_PROPERTY_SHADING_MODE,
                           static_cast<int>(MaterialShadingMode::basic)});
        props.setProperty({MATERIAL_PROPERTY_USER_PARAMETER, 1.0});
        if (surface)
            props.setProperty(
                {MATERIAL_PROPERTY_CHAMELEON_MODE,
                 static_cast<int>(MaterialChameleonMode::emitter)});
        else
            props.setProperty(
                {MATERIAL_PROPERTY_CHAMELEON_MODE,
                 static_cast<int>(
                     MaterialChameleonMode::undefined_chameleon_mode)});

        props.setProperty({MATERIAL_PROPERTY_NODE_ID, static_cast<int>(_uuid)});

        material->setDiffuseColor(
            {rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0});
        material->updateProperties(props);

        // Geometry
        model.addSphere(atom.first, {atom.second.position,
                                     static_cast<float>(atom.second.radius *
                                                        atomRadiusMultiplier)});
    }

    // Bonds
    if (loadBonds)
    {
        PLUGIN_INFO(3, "Building " << _bondsMap.size() << " bonds...");
        for (const auto& bond : _bondsMap)
        {
            const auto& atomSrc = _atomMap.find(bond.first)->second;
            for (const auto bondedAtom : bond.second)
            {
                const auto& atomDst = _atomMap.find(bondedAtom)->second;

                const auto center = (atomSrc.position + atomDst.position) / 2.0;

                model.addCylinder(bond.first,
                                  {atomSrc.position, center,
                                   static_cast<float>(atomRadiusMultiplier *
                                                      BOND_RADIUS)});

                model.addCylinder(bondedAtom,
                                  {atomDst.position, center,
                                   static_cast<float>(atomRadiusMultiplier *
                                                      BOND_RADIUS)});
            }
        }
    }

    // Sticks
    if (representation == ProteinRepresentation::atoms_and_sticks)
    {
        PLUGIN_INFO(3, "Building sticks (" << _atomMap.size() << " atoms)...");
        auto it1 = _atomMap.begin();
        while (it1 != _atomMap.end())
        {
            const auto atom1 = (*it1);
            auto it2 = it1;
            ++it2;
            while ((*it2).second.reqSeq == atom1.second.reqSeq)
            {
                const auto stick =
                    (*it2).second.position - atom1.second.position;
                if (length(stick) < DEFAULT_STICK_DISTANCE)
                {
                    const auto center =
                        ((*it2).second.position + atom1.second.position) / 2.0;
                    model.addCylinder(atom1.first,
                                      {atom1.second.position, center,
                                       static_cast<float>(atomRadiusMultiplier *
                                                          BOND_RADIUS)});
                    model.addCylinder((*it2).first,
                                      {(*it2).second.position, center,
                                       static_cast<float>(atomRadiusMultiplier *
                                                          BOND_RADIUS)});
                }
                ++it2;
                ++it1;
            }
            ++it1;
        }
    }
}

void Molecule::_buildModel(const std::string& assemblyName,
                           const std::string& name, const std::string& title,
                           const std::string& header,
                           const ProteinRepresentation& representation,
                           const double atomRadiusMultiplier,
                           const bool loadBonds)
{
    PLUGIN_INFO(3, "Building protein " << name << "...");

    // Metadata
    ModelMetadata metadata;
    metadata[METADATA_ASSEMBLY] = assemblyName;
    metadata[METADATA_TITLE] = title;
    metadata[METADATA_HEADER] = header;
    metadata[METADATA_ATOMS] = std::to_string(_atomMap.size());
    metadata[METADATA_BONDS] = std::to_string(_bondsMap.size());
    metadata[METADATA_AA_RANGE] = std::to_string(_aminoAcidRange.x) + ":" +
                                  std::to_string(_aminoAcidRange.y);

    const auto& size = _bounds.getSize();
    metadata[METADATA_SIZE] = std::to_string(size.x) + ", " +
                              std::to_string(size.y) + ", " +
                              std::to_string(size.z) + " angstroms";

    for (const auto& sequence : getSequencesAsString())
        metadata[METADATA_AA_SEQUENCE + sequence.first] =
            "[" + std::to_string(sequence.second.size()) + "] " +
            sequence.second;

    switch (representation)
    {
    case ProteinRepresentation::atoms:
    case ProteinRepresentation::atoms_and_sticks:
    {
        auto model = _scene.createModel();
        _buildAtomicStruture(representation, atomRadiusMultiplier, false,
                             loadBonds, *model);
        _modelDescriptor =
            std::make_shared<ModelDescriptor>(std::move(model), name, header,
                                              metadata);
        break;
    }
#ifdef USE_CGAL
    case ProteinRepresentation::surface:
    case ProteinRepresentation::union_of_balls:
    {
        // Surface
        Vector4ds pointCloud;
        const size_t materialId{0};
        for (const auto& atom : _atomMap)
        {
            if (atom.first % std::max(1, int(atomRadiusMultiplier)) != 0)
                continue;
            pointCloud.push_back({atom.second.position.x,
                                  atom.second.position.y,
                                  atom.second.position.z,
                                  atom.second.radius * atomRadiusMultiplier});
        }

        SurfaceMesher sm(_uuid);
        if (representation == ProteinRepresentation::union_of_balls)
            _modelDescriptor =
                sm.generateUnionOfBalls(_scene, name, pointCloud);
        else
            _modelDescriptor = sm.generateSurface(_scene, name, pointCloud);
        _modelDescriptor->setMetadata(metadata);

        Model& model = _modelDescriptor->getModel();
        _buildAtomicStruture(representation, atomRadiusMultiplier * 2.0, true,
                             loadBonds, model);
        break;
    }
    case ProteinRepresentation::contour:
    {
        auto model = _scene.createModel();
        PointCloud pointCloud;
        for (const auto& atom : _atomMap)
            pointCloud[0].push_back(
                {atom.second.position.x, atom.second.position.y,
                 atom.second.position.z,
                 atom.second.radius * atomRadiusMultiplier});

        PointCloudMesher pcm;
        pcm.toConvexHull(*model, pointCloud);
        _modelDescriptor =
            std::make_shared<ModelDescriptor>(std::move(model), name, header,
                                              metadata);
        break;
    }
#else
    case ProteinRepresentation::surface:
    case ProteinRepresentation::union_of_balls:
    case ProteinRepresentation::contour:
        PLUGIN_THROW("CGAL is required to create surfaces");
        break;
#endif
    case ProteinRepresentation::debug:
    {
        auto model = _scene.createModel();
        const size_t materialId = 0;
        auto material = model->createMaterial(materialId, "Debug");
        brayns::PropertyMap props;
        props.setProperty({MATERIAL_PROPERTY_SHADING_MODE,
                           static_cast<int>(MaterialShadingMode::basic)});
        props.setProperty({MATERIAL_PROPERTY_USER_PARAMETER, 1.0});
        props.setProperty(
            {MATERIAL_PROPERTY_CHAMELEON_MODE,
             static_cast<int>(
                 MaterialChameleonMode::undefined_chameleon_mode)});
        material->setDiffuseColor({1.0, 1.0, 1.0});
        material->updateProperties(props);

        brayns::Boxf box;
        for (const auto& atom : _atomMap)
            box.merge({atom.second.position.x, atom.second.position.y,
                       atom.second.position.z});

        const auto halfSize = box.getSize() * 0.5;
        const auto center = box.getCenter();

        const brayns::Vector3d a = {0.0, 0.0, center.z + halfSize.z};
        const brayns::Vector3d b = {0.0, 0.0, center.z - halfSize.z * 0.5};
        const brayns::Vector3d c = {0.0, 0.0, center.z - halfSize.z * 0.51};
        const brayns::Vector3d d = {0.0, 0.0, center.z - halfSize.z};

        model->addSphere(materialId,
                         {a, static_cast<float>(atomRadiusMultiplier * 0.2)});
        model->addCylinder(materialId,
                           {a, b,
                            static_cast<float>(atomRadiusMultiplier * 0.2)});
        model->addCone(materialId,
                       {b, c, static_cast<float>(atomRadiusMultiplier * 0.2),
                        static_cast<float>(atomRadiusMultiplier)});
        model->addCone(materialId,
                       {c, d, static_cast<float>(atomRadiusMultiplier, 0.0)});
        _modelDescriptor =
            std::make_shared<ModelDescriptor>(std::move(model), name, header,
                                              metadata);
        break;
    }
    }

    PLUGIN_INFO(3, "Molecule model successfully built");

    PLUGIN_INFO(3, "---=== Molecule ===--- ");
    PLUGIN_INFO(3, "Assembly name         : " << assemblyName);
    PLUGIN_INFO(3, "Name                  : " << name);
    PLUGIN_INFO(3, "Atom Radius multiplier: " << atomRadiusMultiplier);
    PLUGIN_INFO(3, "Number of atoms       : " << _atomMap.size());
    PLUGIN_INFO(3, "Number of bonds       : " << _bondsMap.size());

    if (_modelDescriptor &&
        !GeneralSettings::getInstance()->getModelVisibilityOnCreation())
        _modelDescriptor->setVisible(false);
}

void Molecule::_readAtom(const std::string& line, const bool loadHydrogen)
{
    // --------------------------------------------------------------------
    // COLUMNS DATA TYPE    FIELD     DEFINITION
    // --------------------------------------------------------------------
    // 1 - 6   Record name  "ATOM "
    // 7 - 11  Integer      serial     Atom serial number
    // 13 - 16 Atom         name       Atom name
    // 17      Character    altLoc     Alternate location indicator
    // 18 - 20 Residue name resName    Residue name
    // 22      Character    chainID    Chain identifier
    // 23 - 26 Integer      resSeq     Residue sequence number
    // 27      AChar        iCode      Code for insertion of residues
    // 31 - 38 Real(8.3)    x          Orthogonal coords for X in angstroms
    // 39 - 46 Real(8.3)    y          Orthogonal coords for Y in Angstroms
    // 47 - 54 Real(8.3)    z          Orthogonal coords for Z in Angstroms
    // 55 - 60 Real(6.2)    occupancy  Occupancy
    // 61 - 66 Real(6.2)    tempFactor Temperature factor
    // 77 - 78 LString(2)   element    Element symbol, right-justified
    // 79 - 80 LString(2)   charge     Charge on the atom
    // --------------------------------------------------------------------

    std::string s = line.substr(21, 1);
    std::string chainId = trim(s);
    if (!_loadChain(static_cast<size_t>(chainId[0] - 64)))
        return;

    const size_t serial = static_cast<size_t>(atoi(line.substr(6, 5).c_str()));

    Atom atom;
    atom.chainId = chainId;

    s = line.substr(12, 4);
    atom.name = trim(s);

    s = line.substr(16, 1);
    atom.altLoc = trim(s);

    s = line.substr(17, 3);
    atom.resName = trim(s);

    _residues.insert(atom.resName);

    atom.reqSeq = static_cast<size_t>(atoi(line.substr(22, 4).c_str()));
    _aminoAcidRange.x = std::min(atom.reqSeq, size_t(_aminoAcidRange.x));
    _aminoAcidRange.y = std::max(atom.reqSeq, size_t(_aminoAcidRange.y));

    atom.iCode = line.substr(26, 1);

    atom.position.x = static_cast<double>(atof(line.substr(30, 8).c_str()));
    atom.position.y = static_cast<double>(atof(line.substr(38, 8).c_str()));
    atom.position.z = static_cast<double>(atof(line.substr(46, 8).c_str()));

    if (line.length() >= 60)
        atom.occupancy = static_cast<double>(atof(line.substr(54, 6).c_str()));

    if (line.length() >= 66)
        atom.tempFactor = static_cast<double>(atof(line.substr(60, 6).c_str()));

    if (line.length() >= 78)
    {
        s = line.substr(76, 2);
        atom.element = trim(s);
        if (s == "H" && !loadHydrogen)
            return;
    }

    if (line.length() >= 80)
    {
        s = line.substr(78, 2);
        atom.charge = trim(s);
    }

    // Convert position from amstrom (10e-10) to nanometers (10e-9)
    atom.position = 0.1 * atom.position;

    // Bounds
    _bounds.merge(atom.position);

    // Convert radius from picometers (10e-12) to nanometers (10e-9)
    atom.radius = 0.001 * DEFAULT_ATOM_RADIUS;
    auto it = atomicRadii.find(atom.element);
    if (it != atomicRadii.end())
        atom.radius = 0.001 * (*it).second;
    else
    {
        it = atomicRadii.find(atom.name);
        if (it != atomicRadii.end())
            atom.radius = 0.001 * (*it).second;
        else
            PLUGIN_DEBUG("[" << atom.element << "]/[" << atom.name
                             << "] not found");
    }

    _atomMap.insert(std::make_pair(serial, atom));
}

void Molecule::_readSequence(const std::string& line)
{
    // -------------------------------------------------------------------------
    // COLUMNS TYPE      FIELD    DEFINITION
    // -------------------------------------------------------------------------
    // 1 - 6   Record name "SEQRES"
    // 8 - 10  Integer   serNum   Serial number of the SEQRES record for the
    //                            current chain. Starts at 1 and increments
    //                            by one each line. Reset to 1 for each
    //                            chain.
    // 12      Character chainID  Chain identifier. This may be any single
    // legal
    //                            character, including a blank which is is
    //                            used if there is only one chain
    // 14 - 17 Integer   numRes   Number of residues in the chain. This
    // value is
    //                            repeated on every record.
    // 20 - 22 String    resName  Residue name
    // 24 - 26 ...
    // -------------------------------------------------------------------------

    std::string s = line.substr(11, 1);

    ResidueSequence& sequence = _residueSequenceMap[s];
    sequence.numRes = static_cast<size_t>(atoi(line.substr(13, 4).c_str()));

    for (size_t i = 19; i < line.length(); i += 4)
    {
        s = line.substr(i, 4);
        s = trim(s);
        if (!s.empty())
            sequence.resNames.push_back(s);
    }
}

void Molecule::_readConnect(const std::string& line)
{
    // -------------------------------------------------------------------------
    // COLUMNS TYPE      FIELD    DEFINITION
    // -------------------------------------------------------------------------
    // 1 - 6   Record name "CONECT"
    // 7 - 11  Integer   serial Atom serial number
    // 12 - 16 Integer   serial Serial number of bonded atom
    // 17 - 21 Integer   serial Serial number of bonded atom
    // 22 - 26 Integer   serial Serial number of bonded atom
    // 27 - 31 Integer   serial Serial number of bonded atom
    // -------------------------------------------------------------------------

    const size_t serial = static_cast<size_t>(atoi(line.substr(6, 5).c_str()));

    if (_atomMap.find(serial) != _atomMap.end())
    {
        auto& bond = _bondsMap[serial];

        for (size_t i = 11; i < line.length(); i += 5)
        {
            std::string s = line.substr(i, 5);
            s = trim(s);
            if (!s.empty())
            {
                const size_t atomSerial = static_cast<size_t>(atoi(s.c_str()));
                if (_atomMap.find(atomSerial) != _atomMap.end())
                    bond.push_back(atomSerial);
            }
        }
    }
}

void Molecule::_readRemark(const std::string& line)
{
    // -------------------------------------------------------------------------
    // COLUMNS TYPE      FIELD     DEFINITION
    // -------------------------------------------------------------------------
    // 1 - 6   Record name "REMARK"
    // 8 - 10  Integer   remarkNum Remark number. It is not an error for
    // remark
    //                             n to exist in an entry when remark n-1
    //                             does not.
    // 13 - 16 String    "ALN"
    // 17 - 18 String    "C"
    // 19 - 22 String    "TRG"
    // 23 - 81 String    Sequence
    // -------------------------------------------------------------------------

    std::string s = line.substr(9, 1);
    if (s != "3")
        return;

    if (line.length() < 23)
        return;

    s = line.substr(12, 3);
    if (s != "ALN")
        return;

    s = line.substr(16, 1);
    if (s != "C")
        return;

    s = line.substr(18, 3);
    if (s != "TRG")
        return;

    s = line.substr(22, line.length() - 23);
    ResidueSequence& sequence = _residueSequenceMap[0];
    if (sequence.resNames.empty())
        sequence.resNames.push_back(s);
    else
        sequence.resNames[0] = sequence.resNames[0] + s;
}

std::string Molecule::_readTitle(const std::string& line)
{
    std::string s = line.substr(11);
    return trim(s);
}

std::string Molecule::_readHeader(const std::string& line)
{
    std::string s = line.substr(11);
    return trim(s);
}

bool Molecule::_loadChain(const size_t chainId)
{
    bool found = true;
    if (!_chainIds.empty())
    {
        found = false;
        for (const auto id : _chainIds)
            if (id == chainId)
            {
                found = true;
                break;
            }
    }
    return found;
}

void Molecule::_setAtomColorScheme()
{
    std::set<size_t> materialId;
    for (const auto& atom : _atomMap)
    {
        const size_t index = static_cast<size_t>(
            std::distance(atomColorMap.begin(),
                          atomColorMap.find(atom.second.element)));
        materialId.insert(index);

        _setMaterialDiffuseColor(atom.first, atomColorMap[atom.second.element]);
    }
    PLUGIN_INFO(3, "Applying atom color scheme (" << materialId.size() << ")");
}

void Molecule::_setAminoAcidSequenceColorScheme(const Palette& palette)
{
    if (palette.size() != 2)
        PLUGIN_THROW("Invalid palette size. 2 colors are expected");

    size_t atomCount = 0;
    for (const auto& sequence : _residueSequenceMap)
    {
        if (_selectedAminoAcidSequence.empty())
        {
            // Range based coloring
            for (auto& atom : _atomMap)
            {
                bool selected = false;
                for (const auto& range : _selectedAminoAcidRanges)
                {
                    selected = (atom.second.reqSeq >= range.x &&
                                atom.second.reqSeq <= range.y);
                    if (selected)
                        break;
                }
                _setMaterialDiffuseColor(atom.first,
                                         selected ? palette[1] : palette[0]);
            }
        }
        else
        {
            // String based coloring
            std::string shortSequence;
            for (const auto& resName : sequence.second.resNames)
                shortSequence += aminoAcidMap[resName].shortName;

            const auto sequencePosition =
                shortSequence.find(_selectedAminoAcidSequence);
            if (sequencePosition != -1)
            {
                PLUGIN_INFO(3, _selectedAminoAcidSequence
                                   << " was found at position "
                                   << sequencePosition);
                size_t minSeq = 1e6;
                size_t maxSeq = 0;
                for (auto& atom : _atomMap)
                {
                    minSeq = std::min(minSeq, atom.second.reqSeq);
                    maxSeq = std::max(maxSeq, atom.second.reqSeq);
                    if (atom.second.reqSeq >= sequencePosition &&
                        atom.second.reqSeq <
                            sequencePosition +
                                _selectedAminoAcidSequence.length())
                    {
                        _setMaterialDiffuseColor(atom.first, palette[1]);
                        ++atomCount;
                    }
                    else
                        _setMaterialDiffuseColor(atom.first, palette[0]);
                }
                PLUGIN_DEBUG(atomCount << "[" << minSeq << "," << maxSeq
                                       << "] atoms where colored");
            }
            else
                PLUGIN_WARN(_selectedAminoAcidSequence << " was not found in "
                                                       << shortSequence);
        }
    }
    PLUGIN_INFO(3, "Applying Amino Acid Sequence color scheme ("
                       << (atomCount > 0 ? "2" : "1") << ")");
}

void Molecule::_setChainColorScheme(const Palette& palette)
{
    std::set<size_t> materialId;
    for (auto& atom : _atomMap)
    {
        const size_t index = static_cast<size_t>(atom.second.chainId[0]) - 64;
        materialId.insert(index);
        _setMaterialDiffuseColor(atom.first, palette[index]);
    }
    PLUGIN_INFO(3, "Applying Chain color scheme (" << materialId.size() << ")");
}

void Molecule::_setResiduesColorScheme(const Palette& palette)
{
    std::set<size_t> materialId;
    for (auto& atom : _atomMap)
    {
        const size_t index = static_cast<size_t>(
            std::distance(_residues.begin(),
                          _residues.find(atom.second.resName)));
        materialId.insert(index);
        _setMaterialDiffuseColor(atom.first, palette[index]);
    }
    PLUGIN_INFO(3,
                "Applying Residues color scheme (" << materialId.size() << ")");
}

void Molecule::_setMaterialDiffuseColor(const size_t atomIndex,
                                        const RGBColorDetails& color)
{
    auto& model = _modelDescriptor->getModel();
    auto material = model.getMaterial(atomIndex);
    if (material)
    {
        material->setDiffuseColor(
            {color.r / 255.0, color.g / 255.0, color.b / 255.0});
        material->commit();
    }
    else
        PLUGIN_THROW("Model has no material for atom " +
                     std::to_string(atomIndex));
}

void Molecule::_setMaterialDiffuseColor(const size_t atomIndex,
                                        const Color& color)
{
    auto& model = _modelDescriptor->getModel();
    try
    {
        auto material = model.getMaterial(atomIndex);
        if (material)
        {
            material->setDiffuseColor(color);
            material->markModified();
        }
        else
            PLUGIN_THROW("Model has no material for atom " +
                         std::to_string(atomIndex));
    }
    catch (const std::runtime_error& e)
    {
        PLUGIN_ERROR(e.what());
    }
}
} // namespace biology
} // namespace bioexplorer
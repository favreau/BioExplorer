/*
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * Copyright 2020-2022 Blue BrainProject / EPFL
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

#include "Node.h"

#include <plugin/common/CommonTypes.h>
#include <plugin/common/UniqueId.h>

#include <brayns/engineapi/Material.h>
#include <brayns/engineapi/Model.h>

namespace bioexplorer
{
namespace common
{
Node::Node()
{
    // Unique ID
    _uuid = UniqueId::get();
}

const ModelDescriptorPtr Node::getModelDescriptor() const
{
    return _modelDescriptor;
}

void Node::_setMaterialExtraAttributes()
{
    auto materials = _modelDescriptor->getModel().getMaterials();
    for (auto& material : materials)
    {
        brayns::PropertyMap props;
        props.setProperty({MATERIAL_PROPERTY_SHADING_MODE,
                           static_cast<int>(MaterialShadingMode::basic)});
        props.setProperty({MATERIAL_PROPERTY_USER_PARAMETER, 1.0});
        props.setProperty({MATERIAL_PROPERTY_CHAMELEON_MODE,
                           static_cast<int>(MaterialChameleonMode::receiver)});
        props.setProperty({MATERIAL_PROPERTY_NODE_ID, static_cast<int>(_uuid)});
        material.second->updateProperties(props);
    }
}

// TODO: Generalise SDF for any type of asset
size_t Node::_addSDFGeometry(SDFMorphologyData& sdfMorphologyData,
                             const SDFGeometry& geometry,
                             const Neighbours& neighbours,
                             const size_t materialId)
{
    const size_t idx = sdfMorphologyData.geometries.size();
    sdfMorphologyData.geometries.push_back(geometry);
    sdfMorphologyData.neighbours.push_back(neighbours);
    sdfMorphologyData.materials.push_back(materialId);
    return idx;
}

size_t Node::_addSphere(const bool useSDF, const Vector3f& position,
                        const float radius, const size_t materialId,
                        const uint64_t userData, Model& model,
                        SDFMorphologyData& sdfMorphologyData,
                        const Neighbours& neighbours,
                        const float displacementRatio)
{
    if (useSDF)
    {
        const Vector3f displacementParams = {std::min(radius, 0.05f),
                                             displacementRatio, 2.0f};
        return _addSDFGeometry(sdfMorphologyData,
                               createSDFSphere(position, radius, userData,
                                               displacementParams),
                               neighbours, materialId);
    }
    return model.addSphere(materialId, {position, radius, userData});
}

size_t Node::_addCone(const bool useSDF, const Vector3f& position,
                      const float radius, const Vector3f& target,
                      const float previousRadius, const size_t materialId,
                      const uint64_t userData, Model& model,
                      SDFMorphologyData& sdfMorphologyData,
                      const Neighbours& neighbours,
                      const float displacementRatio)
{
    if (useSDF)
    {
        const Vector3f displacementParams = {std::min(radius, 0.05f),
                                             displacementRatio, 2.f};
        const auto geom =
            createSDFConePill(position, target, radius, previousRadius,
                              userData, displacementParams);
        return _addSDFGeometry(sdfMorphologyData, geom, neighbours, materialId);
    }
    if (radius == previousRadius)
        return model.addCylinder(materialId,
                                 {position, target, radius, userData});
    return model.addCone(materialId,
                         {position, target, radius, previousRadius, userData});
}

void Node::_finalizeSDFGeometries(Model& model,
                                  SDFMorphologyData& sdfMorphologyData)
{
    const size_t numGeoms = sdfMorphologyData.geometries.size();

    for (size_t i = 0; i < numGeoms; ++i)
    {
        const auto& neighbours = sdfMorphologyData.neighbours[i];
        for (const auto neighbour : neighbours)
            if (neighbour != i)
                sdfMorphologyData.neighbours[neighbour].insert(i);
    }

    for (size_t i = 0; i < numGeoms; ++i)
    {
        // Convert neighbours from set to vector and erase itself from its
        // neighbours
        std::vector<size_t> neighbours;
        const auto& neighSet = sdfMorphologyData.neighbours[i];
        std::copy(neighSet.begin(), neighSet.end(),
                  std::back_inserter(neighbours));
        neighbours.erase(std::remove_if(neighbours.begin(), neighbours.end(),
                                        [i](size_t elem) { return elem == i; }),
                         neighbours.end());

        model.addSDFGeometry(sdfMorphologyData.materials[i],
                             sdfMorphologyData.geometries[i], neighbours);
    }
}

void Node::_createMaterials(const MaterialSet& materialIds, Model& model)
{
    for (const auto materialId : materialIds)
    {
        Vector3f color{1.f, 1.f, 1.f};
        auto nodeMaterial =
            model.createMaterial(materialId, std::to_string(materialId));
        nodeMaterial->setDiffuseColor(color);
        nodeMaterial->setSpecularColor(color);
        nodeMaterial->setSpecularExponent(100.f);
        PropertyMap props;
        props.setProperty({MATERIAL_PROPERTY_SHADING_MODE, 0});
        props.setProperty({MATERIAL_PROPERTY_USER_PARAMETER, 1.0});
        props.setProperty({MATERIAL_PROPERTY_CHAMELEON_MODE, 0});
        props.setProperty({MATERIAL_PROPERTY_CAST_USER_DATA, false});
        props.setProperty({MATERIAL_PROPERTY_NODE_ID, 0});
        nodeMaterial->updateProperties(props);
    }
}

void Node::addSDFDemo(Model& model)
{
    MaterialSet materialIds;
    size_t materialId = 0;
    const bool useSdf = true;
    const double displacement = 10.0;
    SDFMorphologyData sdfMorphologyData;

    auto idx1 =
        _addCone(useSdf, Vector3d(-1, 0, 0), 0.25, Vector3d(0, 0, 0), 0.1,
                 materialId, -1, model, sdfMorphologyData, {}, displacement);
    materialIds.insert(materialId);
    ++materialId;

    auto idx2 = _addCone(useSdf, Vector3d(0, 0, 0), 0.1, Vector3d(1, 0, 0),
                         0.25, materialId, -1, model, sdfMorphologyData, {idx1},
                         displacement);
    materialIds.insert(materialId);
    ++materialId;

    auto idx3 = _addSphere(useSdf, Vector3d(-1, 0, 0), 0.25, materialId, -1,
                           model, sdfMorphologyData, {idx1}, displacement);
    materialIds.insert(materialId);
    ++materialId;

    auto idx4 = _addSphere(useSdf, Vector3d(1, 0, 0), 0.25, materialId, -1,
                           model, sdfMorphologyData, {idx2}, displacement);
    materialIds.insert(materialId);
    ++materialId;

    auto idx5 = _addCone(useSdf, Vector3d(0, 0.25, 0), 0.5, Vector3d(0, 1, 0),
                         0.0, materialId, -1, model, sdfMorphologyData,
                         {idx1, idx2}, displacement);
    materialIds.insert(materialId);

    _createMaterials(materialIds, model);

    _finalizeSDFGeometries(model, sdfMorphologyData);
}

} // namespace common
} // namespace bioexplorer

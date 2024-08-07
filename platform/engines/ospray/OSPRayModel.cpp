/*
    Copyright 2015 - 2024 Blue Brain Project / EPFL

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

#include "OSPRayModel.h"
#include "OSPRayField.h"
#include "OSPRayMaterial.h"
#include "OSPRayProperties.h"
#include "OSPRayUtils.h"
#include "OSPRayVolume.h"

#include <Defines.h>

#include <platform/core/common/Properties.h>
#include <platform/core/common/simulation/AbstractAnimationHandler.h>
#include <platform/core/engineapi/Material.h>
#include <platform/core/engineapi/Scene.h>
#include <platform/core/parameters/AnimationParameters.h>
#include <platform/core/parameters/FieldParameters.h>
#include <platform/core/parameters/GeometryParameters.h>

namespace
{
template <typename VecT>
OSPData allocateVectorData(const std::vector<VecT>& vec, const OSPDataType ospType, const size_t memoryManagementFlags)
{
    const size_t totBytes = vec.size() * sizeof(decltype(vec.back()));
    return ospNewData(totBytes / ::ospray::sizeOf(ospType), ospType, vec.data(), memoryManagementFlags);
}
} // namespace

namespace core
{
namespace engine
{
namespace ospray
{
OSPRayModel::OSPRayModel(AnimationParameters& animationParameters, VolumeParameters& volumeParameters,
                         GeometryParameters& geometryParameters, FieldParameters& fieldParameters)
    : Model(animationParameters, volumeParameters, geometryParameters, fieldParameters)
{
    _ospTransferFunction = ospNewTransferFunction(OSPRAY_TRANSFER_FUNCTION_PROPERTY_TYPE_PIECEWISE_LINEAR);
    if (_ospTransferFunction)
        ospCommit(_ospTransferFunction);
}

OSPRayModel::~OSPRayModel()
{
    ospRelease(_ospTransferFunction);
    ospRelease(_ospSimulationData);

    const auto releaseAndClearGeometry = [](auto& geometryMap)
    {
        for (auto geom : geometryMap)
            ospRelease(geom.second);
        geometryMap.clear();
    };

    releaseAndClearGeometry(_ospSpheres);
    releaseAndClearGeometry(_ospCylinders);
    releaseAndClearGeometry(_ospCones);
    releaseAndClearGeometry(_ospMeshes);
    releaseAndClearGeometry(_ospStreamlines);
    releaseAndClearGeometry(_ospSDFGeometries);
    releaseAndClearGeometry(_ospCurves);
    releaseAndClearGeometry(_ospFields);

    ospRelease(_primaryModel);
    ospRelease(_secondaryModel);
    ospRelease(_boundingBoxModel);
}

void OSPRayModel::setMemoryFlags(const size_t memoryManagementFlags)
{
    _memoryManagementFlags = memoryManagementFlags;
}

void OSPRayModel::buildBoundingBox()
{
    if (_boundingBoxModel)
        return;
    _boundingBoxModel = ospNewModel();

    auto material = createMaterial(BOUNDINGBOX_MATERIAL_ID, "bounding_box");
    material->setDiffuseColor({1, 1, 1});
    material->setEmission(1.f);
    const Vector3f s(0.5f);
    const Vector3f c(0.5f);
    const float radius = 0.005f;
    const Vector3f positions[8] = {
        {c.x - s.x, c.y - s.y, c.z - s.z}, {c.x + s.x, c.y - s.y, c.z - s.z}, //    6--------7
        {c.x - s.x, c.y + s.y, c.z - s.z},                                    //   /|       /|
        {c.x + s.x, c.y + s.y, c.z - s.z},                                    //  2--------3 |
        {c.x - s.x, c.y - s.y, c.z + s.z},                                    //  | |      | |
        {c.x + s.x, c.y - s.y, c.z + s.z},                                    //  | 4------|-5
        {c.x - s.x, c.y + s.y, c.z + s.z},                                    //  |/       |/
        {c.x + s.x, c.y + s.y, c.z + s.z}                                     //  0--------1
    };

    for (size_t i = 0; i < 8; ++i)
        addSphere(BOUNDINGBOX_MATERIAL_ID, Sphere(positions[i], radius));

    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[0], positions[1], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[2], positions[3], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[4], positions[5], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[6], positions[7], radius});

    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[0], positions[2], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[1], positions[3], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[4], positions[6], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[5], positions[7], radius});

    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[0], positions[4], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[1], positions[5], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[2], positions[6], radius});
    addCylinder(BOUNDINGBOX_MATERIAL_ID, {positions[3], positions[7], radius});
}

void OSPRayModel::_addGeometryToModel(const OSPGeometry geometry, const size_t materialId)
{
    switch (materialId)
    {
    case BOUNDINGBOX_MATERIAL_ID:
        ospAddGeometry(_boundingBoxModel, geometry);
        break;
    case SECONDARY_MODEL_MATERIAL_ID:
    {
        if (!_secondaryModel)
            _secondaryModel = ospNewModel();
        ospAddGeometry(_secondaryModel, geometry);
        break;
    }
    default:
        ospAddGeometry(_primaryModel, geometry);
    }
}

OSPGeometry& OSPRayModel::_createGeometry(GeometryMap& map, const size_t materialId, const char* name)
{
    auto& geometry = map[materialId];
    if (geometry)
    {
        ospRemoveGeometry(_primaryModel, geometry);
        ospRelease(geometry);
    }
    geometry = ospNewGeometry(name);

    auto matIt = _materials.find(materialId);
    if (matIt != _materials.end())
    {
        auto material = std::static_pointer_cast<OSPRayMaterial>(matIt->second);
        if (material->getOSPMaterial())
            ospSetMaterial(geometry, material->getOSPMaterial());
    }

    return geometry;
}

void OSPRayModel::_commitSpheres(const size_t materialId)
{
    auto& geometry = _createGeometry(_ospSpheres, materialId, OSPRAY_GEOMETRY_PROPERTY_SPHERES);

    auto data = allocateVectorData(_geometries->_spheres.at(materialId), OSP_FLOAT, _memoryManagementFlags);

    ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_SPHERES, data);
    ospRelease(data);

    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SPHERE_OFFSET_CENTER, static_cast<int>(offsetof(Sphere, center)));
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SPHERE_OFFSET_RADIUS, static_cast<int>(offsetof(Sphere, radius)));
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SPHERE_BYTES_PER_SPHERE, static_cast<int>(sizeof(Sphere)));
    ospCommit(geometry);

    _addGeometryToModel(geometry, materialId);
}

void OSPRayModel::_commitCylinders(const size_t materialId)
{
    auto& geometry = _createGeometry(_ospCylinders, materialId, OSPRAY_GEOMETRY_PROPERTY_CYLINDERS);

    auto data = allocateVectorData(_geometries->_cylinders.at(materialId), OSP_FLOAT, _memoryManagementFlags);
    ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_CYLINDERS, data);
    ospRelease(data);

    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_CYLINDER_OFFSET_V0, static_cast<int>(offsetof(Cylinder, center)));
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_CYLINDER_OFFSET_V1, static_cast<int>(offsetof(Cylinder, up)));
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_CYLINDER_OFFSET_RADIUS,
                   static_cast<int>(offsetof(Cylinder, radius)));
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_CYLINDER_BYTES_PER_CYLINDER, static_cast<int>(sizeof(Cylinder)));
    ospCommit(geometry);

    _addGeometryToModel(geometry, materialId);
}

void OSPRayModel::_commitCones(const size_t materialId)
{
    auto& geometry = _createGeometry(_ospCones, materialId, OSPRAY_GEOMETRY_PROPERTY_CONES);

    auto data = allocateVectorData(_geometries->_cones.at(materialId), OSP_FLOAT, _memoryManagementFlags);
    ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_CONES, data);
    ospRelease(data);

    ospCommit(geometry);

    _addGeometryToModel(geometry, materialId);
}

void OSPRayModel::_commitMeshes(const size_t materialId)
{
    auto& geometry = _createGeometry(_ospMeshes, materialId, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH);
    auto& triangleMesh = _geometries->_triangleMeshes.at(materialId);

    OSPData vertices = allocateVectorData(triangleMesh.vertices, OSP_FLOAT3, _memoryManagementFlags);
    ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH_VERTEX, vertices);
    ospRelease(vertices);

    OSPData indices = allocateVectorData(triangleMesh.indices, OSP_INT3, _memoryManagementFlags);
    ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH_INDEX, indices);
    ospRelease(indices);

    if (!triangleMesh.normals.empty())
    {
        OSPData normals = allocateVectorData(triangleMesh.normals, OSP_FLOAT3, _memoryManagementFlags);
        ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH_NORMAL, normals);
        ospRelease(normals);
    }

    if (!triangleMesh.colors.empty())
    {
        OSPData colors = allocateVectorData(triangleMesh.colors, OSP_FLOAT3A, _memoryManagementFlags);
        ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH_COLOR, colors);
        ospRelease(colors);
    }

    if (!triangleMesh.textureCoordinates.empty())
    {
        OSPData texCoords = allocateVectorData(triangleMesh.textureCoordinates, OSP_FLOAT2, _memoryManagementFlags);
        ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH_TEXTURE_COORDINATES, texCoords);
        ospRelease(texCoords);
    }

    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH_ALPHA_TYPE,
                   OSPRAY_GEOMETRY_DEFAULT_TRIANGLE_MESH_ALPHA_TYPE);
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_TRIANGLE_MESH_ALPHA_COMPONENT,
                   OSPRAY_GEOMETRY_DEFAULT_TRIANGLE_MESH_ALPHA_COMPONENT);
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_EPSILON, _geometryParameters.getSdfEpsilon());
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_NB_MARCH_ITERATIONS,
                   static_cast<int>(_geometryParameters.getSdfNbMarchIterations()));
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_BLEND_FACTOR, _geometryParameters.getSdfBlendFactor());
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_BLEND_LERP_FACTOR,
                   _geometryParameters.getSdfBlendLerpFactor());
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_OMEGA, _geometryParameters.getSdfOmega());
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_DISTANCE, _geometryParameters.getSdfDistance());

    ospCommit(geometry);

    ospAddGeometry(_primaryModel, geometry);
}

void OSPRayModel::_commitStreamlines(const size_t materialId)
{
    auto& geometry = _createGeometry(_ospStreamlines, materialId, OSPRAY_GEOMETRY_PROPERTY_STREAMLINES);
    auto& data = _geometries->_streamlines[materialId];

    {
        OSPData vertex = allocateVectorData(data.vertex, OSP_FLOAT4, _memoryManagementFlags);
        ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_STREAMLINE_VERTEX, vertex);
        ospRelease(vertex);
    }
    {
        OSPData vertexColor = allocateVectorData(data.vertexColor, OSP_FLOAT4, _memoryManagementFlags);
        ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_STREAMLINE_COLOR, vertexColor);
        ospRelease(vertexColor);
    }
    {
        OSPData index = allocateVectorData(data.indices, OSP_INT, _memoryManagementFlags);
        ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_STREAMLINE_INDEX, index);
        ospRelease(index);
    }

    // Since we allow custom radius per point we always smooth
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_STREAMLINE_TYPE_SMOOTH, true);

    ospCommit(geometry);

    ospAddGeometry(_primaryModel, geometry);
}

void OSPRayModel::_commitSDFGeometries()
{
    if (_geometries->_sdf.geometries.empty())
        return;

    auto globalData = allocateVectorData(_geometries->_sdf.geometries, OSP_CHAR, _memoryManagementFlags);

    // Create and upload flat list of neighbours
    const size_t numGeoms = _geometries->_sdf.geometries.size();
    _geometries->_sdf.neighboursFlat.clear();

    for (size_t geomI = 0; geomI < numGeoms; geomI++)
    {
        const size_t currOffset = _geometries->_sdf.neighboursFlat.size();
        const auto& neighsI = _geometries->_sdf.neighbours[geomI];
        if (!neighsI.empty())
        {
            _geometries->_sdf.geometries[geomI].numNeighbours = neighsI.size();
            _geometries->_sdf.geometries[geomI].neighboursIndex = currOffset;
            _geometries->_sdf.neighboursFlat.insert(std::end(_geometries->_sdf.neighboursFlat), std::begin(neighsI),
                                                    std::end(neighsI));
        }
    }

    // Make sure we don't create an empty buffer in the case of no neighbours
    if (_geometries->_sdf.neighboursFlat.empty())
        _geometries->_sdf.neighboursFlat.resize(1, 0);

    auto neighbourData = allocateVectorData(_geometries->_sdf.neighboursFlat, OSP_ULONG, _memoryManagementFlags);

    for (const auto& mat : _materials)
    {
        const size_t materialId = mat.first;

        if (_geometries->_sdf.geometryIndices.find(materialId) == _geometries->_sdf.geometryIndices.end())
            continue;

        auto& geometry = _createGeometry(_ospSDFGeometries, materialId, OSPRAY_GEOMETRY_PROPERTY_SDF);

        auto data =
            allocateVectorData(_geometries->_sdf.geometryIndices[materialId], OSP_ULONG, _memoryManagementFlags);
        ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF, data);
        ospRelease(data);

        ospSetData(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_NEIGHBOURS, neighbourData);
        ospSetData(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_GEOMETRIES, globalData);
        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_EPSILON, _geometryParameters.getSdfEpsilon());
        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_NB_MARCH_ITERATIONS,
                       static_cast<int>(_geometryParameters.getSdfNbMarchIterations()));
        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_BLEND_FACTOR, _geometryParameters.getSdfBlendFactor());
        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_BLEND_LERP_FACTOR,
                       _geometryParameters.getSdfBlendLerpFactor());
        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_OMEGA, _geometryParameters.getSdfOmega());
        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_SDF_DISTANCE, _geometryParameters.getSdfDistance());

        ospCommit(geometry);

        ospAddGeometry(_primaryModel, geometry);
    }

    ospRelease(globalData);
    ospRelease(neighbourData);
}

void OSPRayModel::_commitCurves(const size_t materialId)
{
    const auto& curves = _geometries->_curves[materialId];
    for (const auto& curve : curves)
    {
        auto& geometry = _createGeometry(_ospCurves, materialId, OSPRAY_GEOMETRY_PROPERTY_CURVES);

        {
            auto vertices = allocateVectorData(curve.vertices, OSP_FLOAT4, _memoryManagementFlags);
            ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_CURVE_VERTEX, vertices);
            ospRelease(vertices);
        }
        {
            auto indices = allocateVectorData(curve.indices, OSP_INT, _memoryManagementFlags);
            ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_CURVE_INDEX, indices);
            ospRelease(indices);
        }
        {
            auto normals = allocateVectorData(curve.normals, OSP_FLOAT3, _memoryManagementFlags);
            ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_CURVE_NORMAL, normals);
            ospRelease(normals);
        }
        {
            auto tangents = allocateVectorData(curve.tangents, OSP_FLOAT3, _memoryManagementFlags);
            ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_CURVE_TANGENT, tangents);
            ospRelease(tangents);
        }

        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_CURVE_BASIS, baseTypeAsString(curve.baseType));
        osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_CURVE_TYPE, curveTypeAsString(curve.curveType));

        ospCommit(geometry);

        ospAddGeometry(_primaryModel, geometry);
    }
}

void OSPRayModel::_commitFields(const size_t materialId)
{
    auto& geometry = _createGeometry(_ospFields, materialId, OSPRAY_GEOMETRY_PROPERTY_FIELDS);
    auto& field = _geometries->_fields.at(materialId);

    const auto& indices = field->getOctreeIndices();
    const auto& values = field->getOctreeValues();
    if (indices.empty() || values.empty())
    {
        CORE_ERROR("No values from Octree for fields geometry");
        return;
    }

    CORE_DEBUG("Fields: Committing " << field->getOctreeIndices().size() << " indices");
    auto indicesBuffer = allocateVectorData(field->getOctreeIndices(), OSP_UINT, _memoryManagementFlags);
    ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_FIELD_INDICES, indicesBuffer);
    ospRelease(indicesBuffer);

    CORE_DEBUG("Fields: Committing " << field->getOctreeValues().size() << " values");
    auto valuesBuffer = allocateVectorData(field->getOctreeValues(), OSP_FLOAT, _memoryManagementFlags);
    ospSetObject(geometry, OSPRAY_GEOMETRY_PROPERTY_FIELD_VALUES, valuesBuffer);
    ospRelease(valuesBuffer);

    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_FIELD_DIMENSIONS, field->getDimensions());
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_FIELD_SPACING, field->getElementSpacing());
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_FIELD_OFFSET, field->getOffset());
    osphelper::set(geometry, OSPRAY_GEOMETRY_PROPERTY_FIELD_DATATYPE, static_cast<int>(field->getOctreeDataType()));
    ospSetObject(geometry, DEFAULT_COMMON_TRANSFER_FUNCTION, _ospTransferFunction);

    ospCommit(geometry);
    ospAddGeometry(_primaryModel, geometry);

    _ospFields[materialId] = geometry;
}

void OSPRayModel::commitFieldParameters()
{
    for (auto& field : _geometries->_fields)
    {
        auto& ospField = _ospFields[field.first];
        osphelper::set(ospField, OSPRAY_FIELD_PROPERTY_GRADIENT_SHADING_ENABLED,
                       static_cast<int>(_fieldParameters.getGradientShading()));
        osphelper::set(ospField, OSPRAY_FIELD_PROPERTY_GRADIENT_OFFSET,
                       static_cast<float>(_fieldParameters.getGradientOffset()));
        osphelper::set(ospField, OSPRAY_FIELD_PROPERTY_SAMPLING_RATE,
                       static_cast<float>(_fieldParameters.getSamplingRate()));
        osphelper::set(ospField, OSPRAY_FIELD_PROPERTY_DISTANCE, static_cast<float>(_fieldParameters.getDistance()));
        osphelper::set(ospField, OSPRAY_FIELD_PROPERTY_CUTOFF, static_cast<float>(_fieldParameters.getCutoff()));
        osphelper::set(ospField, OSPRAY_FIELD_PROPERTY_EPSILON, static_cast<float>(_fieldParameters.getEpsilon()));
        osphelper::set(ospField, OSPRAY_GEOMETRY_PROPERTY_FIELD_ACCUMULATION_STEPS,
                       static_cast<int>(_fieldParameters.getAccumulationSteps()));
        osphelper::set(ospField, OSPRAY_GEOMETRY_PROPERTY_FIELD_ACCUMULATION_COUNT,
                       static_cast<int>(++_commitFieldCount));
        ospCommit(ospField);
    }
}

void OSPRayModel::_setBVHFlags()
{
    osphelper::set(_primaryModel, OSPRAY_MODEL_PROPERTY_DYNAMIC_SCENE,
                   static_cast<int>(_bvhFlags.count(BVHFlag::dynamic)));
    osphelper::set(_primaryModel, OSPRAY_MODEL_PROPERTY_COMPACT_MODE,
                   static_cast<int>(_bvhFlags.count(BVHFlag::compact)));
    osphelper::set(_primaryModel, OSPRAY_MODEL_PROPERTY_ROBUST_MODE,
                   static_cast<int>(_bvhFlags.count(BVHFlag::robust)));
}

void OSPRayModel::commitGeometry()
{
    for (auto volume : _geometries->_volumes)
    {
        auto ospVolume = std::dynamic_pointer_cast<OSPRayVolume>(volume.second);
        ospVolume->commit();
    }

    if (!isDirty())
        return;

    if (!_primaryModel)
        _primaryModel = ospNewModel();

    // Materials
    for (auto material : _materials)
        material.second->commit();

    // Group geometry
    if (_spheresDirty)
        for (const auto& spheres : _geometries->_spheres)
            _commitSpheres(spheres.first);

    if (_cylindersDirty)
        for (const auto& cylinders : _geometries->_cylinders)
            _commitCylinders(cylinders.first);

    if (_conesDirty)
        for (const auto& cones : _geometries->_cones)
            _commitCones(cones.first);

    if (_triangleMeshesDirty)
        for (const auto& meshes : _geometries->_triangleMeshes)
            _commitMeshes(meshes.first);

    if (_streamlinesDirty)
        for (const auto& streamlines : _geometries->_streamlines)
            _commitStreamlines(streamlines.first);

    if (_sdfGeometriesDirty)
        _commitSDFGeometries();

    if (_curvesDirty)
        for (const auto& curve : _geometries->_curves)
            _commitCurves(curve.first);

    if (_fieldsDirty)
        for (const auto& field : _geometries->_fields)
            _commitFields(field.first);

    updateBounds();
    _markGeometriesClean();
    _setBVHFlags();

    // handled by the scene
    _instancesDirty = false;

    // Commit models
    ospCommit(_primaryModel);
    if (_secondaryModel)
        ospCommit(_secondaryModel);
    if (_boundingBoxModel)
        ospCommit(_boundingBoxModel);
}

void OSPRayModel::commitMaterials(const std::string& renderer)
{
    if (renderer.empty())
        throw std::runtime_error("Materials cannot be instanced with an empty renderer name");
    if (_renderer != renderer)
    {
        for (auto kv : _materials)
        {
            auto& material = *kv.second;
            static_cast<OSPRayMaterial&>(material).commit(renderer);
        }

        _renderer = renderer;

        for (auto& map : {_ospSpheres, _ospCylinders, _ospCones, _ospMeshes, _ospStreamlines, _ospSDFGeometries})
        {
            auto matIt = _materials.begin();
            auto geomIt = map.begin();
            while (matIt != _materials.end() && geomIt != map.end())
            {
                while (matIt->first < geomIt->first && matIt != _materials.end())
                    ++matIt;
                if (matIt->first != geomIt->first)
                {
                    CORE_ERROR("Material for geometry missing");
                    ++geomIt;
                    continue;
                }
                auto& material = static_cast<OSPRayMaterial&>(*matIt->second);
                ospSetMaterial(geomIt->second, material.getOSPMaterial());
                ospCommit(geomIt->second);
                ++geomIt;
            }
        }
    }
    else
    {
        for (auto kv : _materials)
        {
            auto& material = *kv.second;
            static_cast<OSPRayMaterial&>(material).commit();
        }
    }
}

MaterialPtr OSPRayModel::createMaterialImpl(const PropertyMap& properties)
{
    return std::make_shared<OSPRayMaterial>(properties);
}

SharedDataVolumePtr OSPRayModel::createSharedDataVolume(const Vector3ui& dimensions, const Vector3f& spacing,
                                                        const DataType type)
{
    return std::make_shared<OSPRaySharedDataVolume>(dimensions, spacing, type, _volumeParameters, _ospTransferFunction);
}

BrickedVolumePtr OSPRayModel::createBrickedVolume(const Vector3ui& dimensions, const Vector3f& spacing,
                                                  const DataType type)
{
    return std::make_shared<OSPRayBrickedVolume>(dimensions, spacing, type, _volumeParameters, _ospTransferFunction);
}

FieldPtr OSPRayModel::createField(const Vector3ui& dimensions, const Vector3f& spacing, const Vector3f& offset,
                                  const uint32_ts& indices, const floats& values, const OctreeDataType dataType)
{
    return std::make_shared<OSPRayField>(_fieldParameters, dimensions, spacing, offset, indices, values, dataType);
}

void OSPRayModel::_commitTransferFunctionImpl(const Vector3fs& colors, const floats& opacities,
                                              const Vector2d valueRange)
{
    // Colors
    OSPData colorsData = ospNewData(colors.size(), OSP_FLOAT3, colors.data());
    ospSetData(_ospTransferFunction, OSPRAY_TRANSFER_FUNCTION_PROPERTY_COLORS, colorsData);
    ospRelease(colorsData);

    // Opacities
    OSPData opacityData = ospNewData(opacities.size(), OSP_FLOAT, opacities.data());
    ospSetData(_ospTransferFunction, OSPRAY_TRANSFER_FUNCTION_PROPERTY_OPACITIES, opacityData);
    ospRelease(opacityData);

    // Value range
    osphelper::set(_ospTransferFunction, OSPRAY_TRANSFER_FUNCTION_PROPERTY_VALUE_RANGE, Vector2f(valueRange));

    ospCommit(_ospTransferFunction);
}

void OSPRayModel::_commitSimulationDataImpl(const float* frameData, const size_t frameSize)
{
    ospRelease(_ospSimulationData);
    _ospSimulationData = ospNewData(frameSize, OSP_FLOAT, frameData, _memoryManagementFlags);
    ospCommit(_ospSimulationData);
}
} // namespace ospray
} // namespace engine
} // namespace core
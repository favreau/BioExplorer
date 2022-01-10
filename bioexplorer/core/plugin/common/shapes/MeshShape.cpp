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

#include "MeshShape.h"

#include <plugin/common/Logs.h>
#include <plugin/common/Utils.h>

namespace bioexplorer
{
namespace common
{
using namespace brayns;
using namespace details;

MeshShape::MeshShape(const Vector3d& scale, const Vector4ds& clippingPlanes,
                     const std::string& contents)
    : Shape(clippingPlanes)
{
    // Load mesh
    Assimp::Importer importer;
    const aiScene* aiScene =
        importer.ReadFileFromMemory(contents.c_str(), contents.length(),
                                    aiProcess_GenSmoothNormals |
                                        aiProcess_Triangulate);

    if (!aiScene)
        PLUGIN_THROW(importer.GetErrorString());

    if (!aiScene->HasMeshes())
        PLUGIN_THROW("No mesh found");

    const auto trfm = aiScene->mRootNode->mTransformation;
    const Matrix4f mainTransformation{trfm.a1, trfm.b1, trfm.c1, trfm.d1,
                                      trfm.a2, trfm.b2, trfm.c2, trfm.d2,
                                      trfm.a3, trfm.b3, trfm.c3, trfm.d3,
                                      trfm.a4, trfm.b4, trfm.c4, trfm.d4};
    // Add protein instances according to membrane topology
    for (size_t m = 0; m < aiScene->mNumMeshes; ++m)
    {
        aiMesh* mesh = aiScene->mMeshes[m];

        // Determine mesh center
        Vector3d meshCenter{0.0, 0.0, 0.0};
        for (uint64_t i = 0; i < mesh->mNumVertices; ++i)
        {
            const auto& v = mesh->mVertices[i];
            meshCenter += _toVector3d(v);
        }
        meshCenter /= mesh->mNumVertices;

        // Recenter mesh and store transformed vertices
        for (uint64_t i = 0; i < mesh->mNumVertices; ++i)
        {
            const auto v = _toVector3d(mesh->mVertices[i], meshCenter, scale);
            _vertices.push_back(v);
            _bounds.merge(v);
        }

        // Store faces
        for (size_t f = 0; f < mesh->mNumFaces; ++f)
            if (mesh->mFaces[f].mNumIndices == 3)
            {
                const Vector3ui face(mesh->mFaces[f].mIndices[0],
                                     mesh->mFaces[f].mIndices[1],
                                     mesh->mFaces[f].mIndices[2]);

                _faces.push_back(face);
                const auto faceSurface =
                    _getSurfaceArea(_vertices[face.x], _vertices[face.y],
                                    _vertices[face.z]);
                _faceSurfaces.push_back(faceSurface);
                _surface += faceSurface;
            }

        // Store normals
        _normals.resize(_vertices.size());
        if (mesh->HasNormals())
        {
            for (const auto& face : _faces)
            {
                auto normal =
                    glm::normalize(_toVector3d(mesh->mNormals[face.x]));
                _normals[face.x] = normal;
                normal = glm::normalize(_toVector3d(mesh->mNormals[face.y]));
                _normals[face.y] = normal;
                normal = glm::normalize(_toVector3d(mesh->mNormals[face.z]));
                _normals[face.z] = normal;
            }
        }
        else
            for (const auto& face : _faces)
            {
                const auto v0 =
                    glm::normalize(_vertices[face.y] - _vertices[face.x]);
                const auto v1 =
                    glm::normalize(_vertices[face.z] - _vertices[face.x]);
                const auto normal = glm::cross(v0, v1);
                _normals[face.x] = normal;
                _normals[face.y] = normal;
                _normals[face.z] = normal;
            }

        PLUGIN_INFO("----===  MeshBasedMembrane  ===----");
        PLUGIN_INFO("Scale                : " << scale);
        PLUGIN_INFO("Number of faces      : " << _faces.size());
        PLUGIN_INFO("Mesh surface area    : " << _surface);
        PLUGIN_INFO("Bounds               : " << _bounds);
    }
}

Transformation MeshShape::getTransformation(
    const uint64_t occurrence, const uint64_t nbOccurrences,
    const AnimationDetails& animationDetails, const double offset) const
{
    const double instanceSurface = _surface / nbOccurrences;
    const double expectedSurfaceCovering = instanceSurface * occurrence;
    double surfaceCoveringProcess = 0.0;
    uint64_t faceIndex = 0;
    while (true)
    {
        surfaceCoveringProcess += _faceSurfaces[faceIndex];
        if (surfaceCoveringProcess >= expectedSurfaceCovering)
            break;
        ++faceIndex;
    }

    const auto face = _faces[faceIndex];

    Vector2d coordinates{1.0, 1.0};
    while (coordinates.x + coordinates.y > 1.0)
    {
        coordinates.x = 0.5f + rnd1();
        coordinates.y = 0.5f + rnd1();
    }

    const auto v00 = _vertices[face.y] - _vertices[face.x];
    const auto v01 = _vertices[face.z] - _vertices[face.x];
    Vector3d pos =
        _vertices[face.x] + v00 * coordinates.x + v01 * coordinates.y;

    // Clipping planes
    if (isClipped(pos, _clippingPlanes))
        throw std::runtime_error("Instance is clipped");

    // Normals
    const auto v10 = _vertices[face.x] - pos;
    const auto v11 = _vertices[face.y] - pos;
    const auto v12 = _vertices[face.z] - pos;
    const Vector3d areas{0.5 * length(glm::cross(v11, v12)),
                         0.5 * length(glm::cross(v10, v12)),
                         0.5 * length(glm::cross(v10, v11))};

    const auto n0 = _normals[face.x];
    const auto n1 = _normals[face.y];
    const auto n2 = _normals[face.z];

    const Vector3d normal = glm::normalize(
        Vector4d(glm::normalize((n0 * areas.x + n1 * areas.y + n2 * areas.z) /
                                (areas.x + areas.y + areas.z)),
                 0.0));

    Quaterniond rot = safeQuatlookAt(normal);
    if (animationDetails.positionSeed != 0)
        pos += animationDetails.positionStrength *
               Vector3d(rnd2(animationDetails.positionSeed),
                        rnd2(animationDetails.positionSeed + 1),
                        rnd2(animationDetails.positionSeed + 2));

    if (animationDetails.rotationSeed != 0)
        rot = weightedRandomRotation(rot, animationDetails.rotationSeed,
                                     occurrence,
                                     animationDetails.rotationStrength);

    pos += offset * normal;

    Transformation transformation;
    transformation.setTranslation(pos);
    transformation.setRotation(rot);
    return transformation;
}

bool MeshShape::isInside(const Vector3d& point) const
{
    const Vector3d& center = _bounds.getCenter();
    const Vector3d rayDirection = center - point;
    const double rayLength = length(rayDirection);
    const Vector3d direction = normalize(rayDirection);
    for (const auto& face : _faces)
    {
        Boxf box;
        box.merge(_vertices[face.x]);
        box.merge(_vertices[face.y]);
        box.merge(_vertices[face.z]);
        if (_rayBoxIntersection(point, direction, box, rayLength / 10.0,
                                rayLength))
            return false;
    }
    return true;
}

bool MeshShape::_rayBoxIntersection(const Vector3d& origin,
                                    const Vector3d& direction, const Boxf& box,
                                    const double t0, const double t1) const
{
    const Vector3d bounds[2]{box.getMin(), box.getMax()};
    const Vector3d invDir = 1.0 / direction;
    const Vector3ui sign{invDir.x < 0.0, invDir.y < 0.0, invDir.z < 0.0};

    double tmin, tmax, tymin, tymax, tzmin, tzmax;

    tmin = (bounds[sign.x].x - origin.x) * invDir.x;
    tmax = (bounds[1 - sign.x].x - origin.x) * invDir.x;
    tymin = (bounds[sign.y].y - origin.y) * invDir.y;
    tymax = (bounds[1 - sign.y].y - origin.y) * invDir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    tzmin = (bounds[sign.z].z - origin.z) * invDir.z;
    tzmax = (bounds[1 - sign.z].z - origin.z) * invDir.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    return (tmin < t1 && tmax > t0);
}

Vector3d MeshShape::_toVector3d(const aiVector3D& v) const
{
    return Vector3d(v.x, v.y, v.z);
}

Vector3d MeshShape::_toVector3d(const aiVector3D& v, const Vector3d& center,
                                const Vector3d& scale) const
{
    const Vector3d p{v.x, v.y, v.z};
    const Vector3d a = p - center;
    const Vector3d b = (p + a) * scale;
    return b;
}

Vector3d MeshShape::_toVector3d(const aiVector3D& v, const Vector3d& center,
                                const Vector3d& scale,
                                const Quaterniond& rotation) const
{
    const Vector3d p{v.x, v.y, v.z};
    const Vector3d a = p - center;
    const Vector3d b = Vector3d(rotation * Vector3d(p + a)) * scale;
    return b;
}

double MeshShape::_getSurfaceArea(const Vector3d& v0, const Vector3d& v1,
                                  const Vector3d& v2) const
{
    // Compute triangle area
    const double a = length(v1 - v0);
    const double b = length(v2 - v0);
    const double c = length(v2 - v1);
    const double s = (a + b + c) / 2.0;
    const double e = s * (s - a) * (s - b) * (s - c);
    return sqrt(e);
}

} // namespace common
} // namespace bioexplorer

/*
 * Copyright 2015-2023 Blue Brain Project / EPFL
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
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

#include <platform/core/common/loader/Loader.h>
#include <platform/core/parameters/GeometryParameters.h>

struct aiScene;

namespace core
{
/** Loads meshes from files using the assimp library
 * http://assimp.sourceforge.net
 */
class MeshLoader : public Loader
{
public:
    MeshLoader(Scene& scene);
    MeshLoader(Scene& scene, const GeometryParameters& geom);

    std::vector<std::string> getSupportedStorage() const final;
    std::string getName() const final;
    PropertyMap getProperties() const final;

    bool isSupported(const std::string& storage, const std::string& extension) const final;

    ModelDescriptorPtr importFromStorage(const std::string& storage, const LoaderProgress& callback,
                                         const PropertyMap& properties) const final;

    ModelDescriptorPtr importFromBlob(Blob&& blob, const LoaderProgress& callback,
                                      const PropertyMap& properties) const final;

    ModelMetadata importMesh(const std::string& fileName, const LoaderProgress& callback, Model& model,
                             const Matrix4f& transformation, const size_t defaultMaterialId,
                             const GeometryQuality geometryQuality) const;

private:
    PropertyMap _defaults;

    void _createMaterials(Model& model, const aiScene* aiScene, const std::string& folder) const;

    ModelMetadata _postLoad(const aiScene* aiScene, Model& model, const Matrix4f& transformation,
                            const size_t defaultMaterial, const std::string& folder,
                            const LoaderProgress& callback) const;
    size_t _getQuality(const GeometryQuality geometryQuality) const;
};
} // namespace core

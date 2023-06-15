/*
 * Copyright 2020-2023 Blue Brain Project / EPFL
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

#include <plugin/api/SonataExplorerParams.h>

#include <platform/core/common/Types.h>
#include <platform/core/common/loader/Loader.h>

#include <set>
#include <vector>

namespace sonataexplorer
{
namespace io
{
namespace loader
{
using namespace core;

namespace servus
{
class URI;
}

/**
 * Load circuit from BlueConfig or CircuitConfig file, including simulation.
 */
class BrickLoader : public Loader
{
public:
    BrickLoader(Scene& scene, PropertyMap&& loaderParams = {});

    std::string getName() const final;

    std::vector<std::string> getSupportedExtensions() const final;

    bool isSupported(const std::string& filename, const std::string& extension) const final;

    static PropertyMap getCLIProperties();

    PropertyMap getProperties() const final;

    ModelDescriptorPtr importFromBlob(Blob&& blob, const LoaderProgress& callback,
                                      const PropertyMap& properties) const final;

    ModelDescriptorPtr importFromFile(const std::string& filename, const LoaderProgress& callback,
                                      const PropertyMap& properties) const final;

    void exportToFile(const ModelDescriptorPtr modelDescriptor, const std::string& filename);

private:
    std::string _readString(std::ifstream& f) const;
    PropertyMap _defaults;
};
} // namespace loader
} // namespace io
} // namespace sonataexplorer
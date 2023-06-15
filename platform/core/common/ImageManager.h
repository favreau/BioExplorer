/*
 * Copyright (c) 2015-2023, EPFL/Blue Brain Project
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <platform/core/common/Types.h>
#include <platform/core/common/material/Texture2D.h>

namespace core
{
/**
 * @brief The ImageManager class uses the FreeImage libray to manipulate
 * images. This class provide an API for encoding into a specific format (PNG,
 * JPEG, etc), and exporting frame buffers to the file system
 */
class ImageManager
{
public:
    /**
     * @brief Import a Texture from file
     * @param filename Full name of the texture file
     * @return Pointer to Texture2D object is import was successful, nullptr
     * otherwise
     */
    static Texture2DPtr importTextureFromFile(const std::string& filename, const TextureType type);
};
} // namespace core
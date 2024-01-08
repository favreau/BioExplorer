/*
 * Copyright (c) 2015-2024, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Daniel Nachbaur <daniel.nachbaur@epfl.ch>
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

#include "SharedDataVolume.h"

#include <platform/core/common/Logs.h>

#include <fcntl.h>
#include <fstream>
#include <future>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
const int NO_DESCRIPTOR = -1;
}

namespace core
{
SharedDataVolume::~SharedDataVolume()
{
    if (_memoryMapPtr)
    {
        ::munmap((void*)_memoryMapPtr, _size);
        _memoryMapPtr = nullptr;
    }
    if (_cacheFileDescriptor != NO_DESCRIPTOR)
    {
        ::close(_cacheFileDescriptor);
        _cacheFileDescriptor = NO_DESCRIPTOR;
    }
}

void SharedDataVolume::mapData(const std::string& filename)
{
    _cacheFileDescriptor = open(filename.c_str(), O_RDONLY);
    if (_cacheFileDescriptor == NO_DESCRIPTOR)
        throw std::runtime_error("Failed to open volume file " + filename);

    struct stat sb;
    if (::fstat(_cacheFileDescriptor, &sb) == NO_DESCRIPTOR)
    {
        ::close(_cacheFileDescriptor);
        _cacheFileDescriptor = NO_DESCRIPTOR;
        throw std::runtime_error("Failed to open volume file " + filename);
    }

    _size = sb.st_size;
    _memoryMapPtr = ::mmap(0, _size, PROT_READ, MAP_PRIVATE, _cacheFileDescriptor, 0);
    if (_memoryMapPtr == MAP_FAILED)
    {
        _memoryMapPtr = nullptr;
        ::close(_cacheFileDescriptor);
        _cacheFileDescriptor = NO_DESCRIPTOR;
        throw std::runtime_error("Failed to open volume file " + filename);
    }

    setVoxels(_memoryMapPtr);
}

void SharedDataVolume::mapData(const uint8_ts& buffer)
{
    _memoryBuffer.insert(_memoryBuffer.begin(), buffer.begin(), buffer.end());
    setVoxels(_memoryBuffer.data());
}

void SharedDataVolume::mapData(uint8_ts&& buffer)
{
    _memoryBuffer = std::move(buffer);
    setVoxels(_memoryBuffer.data());
}
} // namespace core

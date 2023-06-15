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

#include "SolRTypes.h"

#include <mutex>

#ifdef USE_CUDA
#include <engines/CudaKernel.h>
#endif
#ifdef USE_OPENCL
#include <engines/OpenCLKernel.h>
#endif

namespace core
{
using namespace solr;

class SolRContext
{
public:
    ~SolRContext();
    static SolRContext& get();

    GPUKernelPtr getKernel();

    std::unique_lock<std::mutex> getScopeLock() { return std::unique_lock<std::mutex>(_mutex); }

private:
    SolRContext();

    static std::unique_ptr<SolRContext> _context;

    GPUKernelPtr _kernel{nullptr};

    std::mutex _mutex;
};
} // namespace core

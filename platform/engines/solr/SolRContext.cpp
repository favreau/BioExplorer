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

#include "SolRContext.h"
#include "Logs.h"

#ifdef USE_CUDA
#include <engines/CudaKernel.h>
#else
#ifdef USE_OPENCL
#include <engines/OpenCLKernel.h>
#endif // USE_OPENCL
#endif // USE_CUDA

namespace core
{
using namespace solr;

std::unique_ptr<SolRContext> SolRContext::_context;

SolRContext::SolRContext() {}

SolRContext::~SolRContext()
{
    if (_kernel)
    {
        _kernel->cleanup();
        _kernel.reset();
        _kernel = nullptr;
    }
}

SolRContext& SolRContext::get()
{
    if (!_context)
        _context.reset(new SolRContext);

    return *_context;
}

GPUKernelPtr SolRContext::getKernel()
{
    if (_kernel)
        return _kernel;

#ifdef USE_CUDA
    PLUGIN_INFO("Initializing SolR with CUDA engine");
    _kernel = GPUKernelPtr(new solr::CudaKernel());
#else
#ifdef USE_OPENCL
    PLUGIN_INFO("Initializing SolR with OpenCL engine");
    _kernel = GPUKernelPtr(new solr::OpenCLKernel());
#else
    PLUGIN_THROW("Sol-R engine is undefined");
#endif // USE_OPENCL
#endif // USE_CUDA
    return _kernel;
}

} // namespace core

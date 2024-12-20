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

#include "GeometryParameters.h"
#include <platform/core/common/Logs.h>
#include <platform/core/common/Types.h>

namespace
{
const std::string PARAM_MEMORY_MODE = "memory-mode";
const std::string PARAM_DEFAULT_BVH_FLAG = "default-bvh-flag";
const std::string PARAM_GEOMETRY_QUALITY = "geometry-quality";
const std::string PARAM_DEFAULT_SDF_EPSILON = "sdf-epsilon";
const std::string PARAM_DEFAULT_SDF_NB_MARCH_ITERATIONS = "sdf-nb-march-iterations";
const std::string PARAM_DEFAULT_SDF_BLEND_FACTOR = "sdf-blend-factor";
const std::string PARAM_DEFAULT_SDF_BLEND_LERP_FACTOR = "sdf-blend-lerp-factor";
const std::string PARAM_DEFAULT_SDF_OMEGA = "sdf-omega";
const std::string PARAM_DEFAULT_SDF_DISTANCE = "sdf-distance";

const std::string GEOMETRY_QUALITIES[3] = {"low", "medium", "high"};
const std::string GEOMETRY_MEMORY_MODES[2] = {"shared", "replicated"};
const std::map<std::string, core::BVHFlag> BVH_TYPES = {{"dynamic", core::BVHFlag::dynamic},
                                                        {"compact", core::BVHFlag::compact},
                                                        {"robust", core::BVHFlag::robust}};
} // namespace

namespace core
{
GeometryParameters::GeometryParameters()
    : AbstractParameters("Geometry")
{
    _parameters.add_options()
        //
        (PARAM_DEFAULT_SDF_EPSILON.c_str(), po::value<float>(), "Signed distance fields geometry epsilon [float]")
        //
        (PARAM_DEFAULT_SDF_NB_MARCH_ITERATIONS.c_str(), po::value<uint64_t>(),
         "Signed distance fields geometry number of ray-marching iterations [int]")
        //
        (PARAM_DEFAULT_SDF_BLEND_FACTOR.c_str(), po::value<float>(),
         "Signed distance fields geometry blending factor [float]")
        //
        (PARAM_DEFAULT_SDF_BLEND_LERP_FACTOR.c_str(), po::value<float>(),
         "Signed distance fields geometry lerp blending factor [float]")
        //
        (PARAM_DEFAULT_SDF_OMEGA.c_str(), po::value<float>(),
         "Signed distance fields geometry ray-marching omega [float]")
        //
        (PARAM_DEFAULT_SDF_DISTANCE.c_str(), po::value<float>(),
         "Distance until which Signed distance fields geometries are processed (blending and displacement) [float]")
        //
        (PARAM_GEOMETRY_QUALITY.c_str(), po::value<std::string>(), "Geometry rendering quality [low|medium|high]")
        //
        (PARAM_MEMORY_MODE.c_str(), po::value<std::string>(),
         "Defines what memory mode should be used between Core and "
         "the underlying renderer [shared|replicated]")
        //
        (PARAM_DEFAULT_BVH_FLAG.c_str(), po::value<std::vector<std::string>>()->multitoken(),
         "Set a default flag to apply to BVH creation, one of "
         "[dynamic|compact|robust], may appear multiple times.");
}

void GeometryParameters::parse(const po::variables_map& vm)
{
    if (vm.count(PARAM_GEOMETRY_QUALITY))
    {
        _geometryQuality = GeometryQuality::low;
        const auto& geometryQuality = vm[PARAM_GEOMETRY_QUALITY].as<std::string>();
        for (size_t i = 0; i < sizeof(GEOMETRY_QUALITIES) / sizeof(GEOMETRY_QUALITIES[0]); ++i)
            if (geometryQuality == GEOMETRY_QUALITIES[i])
                _geometryQuality = static_cast<GeometryQuality>(i);
    }
    if (vm.count(PARAM_DEFAULT_SDF_EPSILON))
        _sdfEpsilon = vm[PARAM_DEFAULT_SDF_EPSILON].as<float>();
    if (vm.count(PARAM_DEFAULT_SDF_BLEND_FACTOR))
        _sdfBlendFactor = vm[PARAM_DEFAULT_SDF_BLEND_FACTOR].as<float>();
    if (vm.count(PARAM_DEFAULT_SDF_BLEND_LERP_FACTOR))
        _sdfBlendLerpFactor = vm[PARAM_DEFAULT_SDF_BLEND_LERP_FACTOR].as<float>();
    if (vm.count(PARAM_DEFAULT_SDF_OMEGA))
        _sdfOmega = vm[PARAM_DEFAULT_SDF_OMEGA].as<float>();
    if (vm.count(PARAM_DEFAULT_SDF_DISTANCE))
        _sdfDistance = vm[PARAM_DEFAULT_SDF_DISTANCE].as<float>();
    if (vm.count(PARAM_MEMORY_MODE))
    {
        const auto& memoryMode = vm[PARAM_MEMORY_MODE].as<std::string>();
        for (size_t i = 0; i < sizeof(GEOMETRY_MEMORY_MODES) / sizeof(GEOMETRY_MEMORY_MODES[0]); ++i)
            if (memoryMode == GEOMETRY_MEMORY_MODES[i])
                _memoryMode = static_cast<MemoryMode>(i);
    }
    if (vm.count(PARAM_DEFAULT_BVH_FLAG))
    {
        const auto& bvhs = vm[PARAM_DEFAULT_BVH_FLAG].as<std::vector<std::string>>();
        for (const auto& bvh : bvhs)
        {
            const auto kv = BVH_TYPES.find(bvh);
            if (kv != BVH_TYPES.end())
                _defaultBVHFlags.insert(kv->second);
            else
                throw std::runtime_error("Invalid bvh flag '" + bvh + "'.");
        }
    }

    markModified();
}

void GeometryParameters::print()
{
    AbstractParameters::print();
    CORE_INFO("Geometry quality                               : "
              << GEOMETRY_QUALITIES[static_cast<size_t>(_geometryQuality)]);
    CORE_INFO("SDF geometry epsilon                           : " << _sdfEpsilon);
    CORE_INFO("SDF geometry number of ray-marching iterations : " << _sdfNbMarchIterations);
    CORE_INFO("SDF geometry blend factor                      : " << _sdfBlendFactor);
    CORE_INFO("SDF geometry blend factor (lerp)               : " << _sdfBlendLerpFactor);
    CORE_INFO("SDF geometry ray-marching omega                : " << _sdfOmega);
    CORE_INFO("SDF geometry distance                          : " << _sdfDistance);
    CORE_INFO("Memory mode                                    : "
              << (_memoryMode == MemoryMode::shared ? "Shared" : "Replicated"));
}
} // namespace core

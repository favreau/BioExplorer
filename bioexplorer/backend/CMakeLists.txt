# Copyright 2020 - 2024 Blue Brain Project / EPFL
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ==============================================================================
# Project
# ==============================================================================
set(LIBRARY_NAME BioExplorer)

# ==============================================================================
# Packages and settings
# ==============================================================================
option(${NAME}_USE_CGAL "Use CGAL meshing features" ON)

# Packages
find_package(OpenMP)
find_package(PQXX REQUIRED)

# Compiler flags
add_compile_options("-fopenmp")

include_directories(
    ${PROJECT_SOURCE_DIR}/bioexplorer/backend
    ${BIOEXPLORER_SOURCE_DIRS}
)

# ==============================================================================
# Sources
# ==============================================================================
set(${NAME}_SOURCES
    science/common/GeneralSettings.cpp
    science/common/Utils.cpp
    science/common/UniqueId.cpp
    science/common/Node.cpp
    science/common/SDFGeometries.cpp
    science/common/shapes/Shape.cpp
    science/common/shapes/SphereShape.cpp
    science/common/shapes/SphericalCellDiffusionShape.cpp
    science/common/shapes/HelixShape.cpp
    science/common/shapes/PlaneShape.cpp
    science/common/shapes/CubeShape.cpp
    science/common/shapes/BezierShape.cpp
    science/common/shapes/SinusoidShape.cpp
    science/common/shapes/FanShape.cpp
    science/common/shapes/RNAShape.cpp
    science/common/shapes/PointShape.cpp
    science/common/Assembly.cpp
    science/common/ThreadSafeContainer.cpp
    science/fields/PointFieldBuilder.cpp
    science/fields/VectorFieldBuilder.cpp
    science/io/CacheLoader.cpp
    science/io/OOCManager.cpp
    science/io/db/DBConnector.cpp
    science/io/cache/MemoryCache.cpp
    science/molecularsystems/EnzymeReaction.cpp
    science/molecularsystems/Molecule.cpp
    science/molecularsystems/Membrane.cpp
    science/molecularsystems/Glycans.cpp
    science/molecularsystems/Protein.cpp
    science/molecularsystems/RNASequence.cpp
    science/morphologies/Morphologies.cpp
    science/atlas/Atlas.cpp
    science/atlas/AtlasLoader.cpp
    science/morphologies/Synapses.cpp
    science/morphologies/AstrocytesLoader.cpp
    science/morphologies/NeuronsLoader.cpp
    science/morphologies/Astrocytes.cpp
    science/morphologies/Neurons.cpp
    science/morphologies/SpikeSimulationHandler.cpp
    science/morphologies/SomaSimulationHandler.cpp
    science/morphologies/CompartmentSimulationHandler.cpp
    science/vasculature/VasculatureHandler.cpp
    science/vasculature/VasculatureLoader.cpp
    science/vasculature/Vasculature.cpp
    science/connectomics/synaptome/Synaptome.cpp
    science/connectomics/whitematter/WhiteMatter.cpp
    science/connectomics/whitematter/WhiteMatterLoader.cpp
    science/connectomics/synapses/SynapseEfficacy.cpp
    science/connectomics/synapses/SynapseEfficacySimulationHandler.cpp
    science/meshing/PointCloudMesher.cpp
    science/meshing/SurfaceMesher.cpp
    science/api/Params.cpp
    science/BioExplorerPlugin.cpp
)

set(${NAME}_PUBLIC_HEADERS
    science/io/OOCManager.h
    science/io/CacheLoader.h
    science/io/cache/MemoryCache.h
    science/io/db/DBConnector.h
    science/common/Assembly.h
    science/common/Node.h
    science/common/SDFGeometries.h
    science/molecularsystems/EnzymeReaction.h
    science/molecularsystems/Molecule.h
    science/molecularsystems/Glycans.h
    science/molecularsystems/Membrane.h
    science/molecularsystems/Protein.h
    science/molecularsystems/RNASequence.h
    science/atlas/Atlas.h
    science/atlas/AtlasLoader.h
    science/morphologies/Synapses.h
    science/morphologies/Morphologies.h
    science/morphologies/AstrocytesLoader.h
    science/morphologies/NeuronsLoader.h
    science/morphologies/Astrocytes.h
    science/morphologies/Neurons.h
    science/morphologies/SpikeSimulationHandler.h
    science/morphologies/SomaSimulationHandler.h
    science/morphologies/CompartmentSimulationHandler.h
    science/vasculature/VasculatureHandler.h
    science/vasculature/VasculatureLoader.h
    science/vasculature/Vasculature.h
    science/connectomics/synaptome/Synaptome.h
    science/connectomics/whitematter/WhiteMatter.h
    science/connectomics/whitematter/WhiteMatterLoader.h
    science/connectomics/synapses/SynapseEfficacy.h
    science/connectomics/synapses/SynapseEfficacySimulationHandler.h
    science/common/UniqueId.h
    science/common/GeneralSettings.h
    science/common/CommonTypes.h
    science/common/ThreadSafeContainer.h
    science/common/shapes/Shape.h
    science/common/shapes/SphereShape.h
    science/common/shapes/SphericalCellDiffusionShape.h
    science/common/shapes/HelixShape.h
    science/common/shapes/PlaneShape.h
    science/common/shapes/CubeShape.h
    science/common/shapes/BezierShape.h
    science/common/shapes/SinusoidShape.h
    science/common/shapes/FanShape.h
    science/common/shapes/RNAShape.h
    science/common/shapes/PointShape.h
    science/meshing/PointCloudMesher.h
    science/fields/FieldBuilder.h
    science/fields/PointFieldBuilder.h
    science/fields/VectorFieldBuilder.h
    science/meshing/SurfaceMesher.h
    science/api/Params.h
    science/BioExplorerPlugin.h
)

if(${CGAL_FOUND} AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-frounding-math)
endif()

if(ASSIMP_FOUND)
    list(APPEND ${NAME}_SOURCES science/common/shapes/MeshShape.cpp)
    list(APPEND ${NAME}_PUBLIC_HEADERS science/common/shapes/MeshShape.h)
    list(APPEND ${NAME}_PRIVATE_MODULE_LIBRARIES ${ASSIMP_LIBRARIES})
endif()

set(${NAME}_PUBLIC_MODULE_LIBRARIES)
set(${NAME}_PRIVATE_MODULE_LIBRARIES)
# ==============================================================================
# OSPRay module
# ==============================================================================
if(PLATFORM_OSPRAY_ENABLED)
    list(APPEND ${NAME}_SOURCES
        module/ispc/renderer/artistic/GolgiStyleRenderer.cpp
        module/ispc/renderer/DensityRenderer.cpp
        module/ispc/renderer/PathTracingRenderer.cpp
        module/ispc/renderer/VoxelRenderer.cpp
    )

    set(${NAME}_ISPC_SOURCES
        module/ispc/renderer/artistic/GolgiStyleRenderer.ispc
        module/ispc/renderer/DensityRenderer.ispc
        module/ispc/renderer/PathTracingRenderer.ispc
        module/ispc/renderer/VoxelRenderer.ispc
    )

    # ==============================================================================
    # Compile ispc code
    # ==============================================================================
    list(APPEND CMAKE_MODULE_PATH ${OSPRAY_CMAKE_ROOT})
    list(APPEND ALL_ISPC_INCLUDES ${BIOEXPLORER_SOURCE_DIRS})
    list(APPEND ALL_ISPC_INCLUDES ${${NAME}_MODULES_DIR})
    list(APPEND ALL_ISPC_INCLUDES ${OSPRAY_INCLUDE_DIRS})
    list(APPEND ALL_ISPC_INCLUDES ${GLM_INCLUDE_DIRS})
    include_directories_ispc(${ALL_ISPC_INCLUDES})
    ospray_ispc_compile(${${NAME}_ISPC_SOURCES})
    list(APPEND ${NAME}_SOURCES ${ISPC_OBJECTS})

    list(APPEND ${NAME}_PUBLIC_MODULE_LIBRARIES ${OSPRAY_LIBRARIES})
    list(APPEND ${NAME}_PRIVATE_MODULE_LIBRARIES OSPRayEngine)
    include_directories(${OSPRAY_INCLUDE_DIRS})
endif()

# ==============================================================================
# OptiX 6 module
# ==============================================================================
if(PLATFORM_OPTIX6_ENABLED)
    list(APPEND CMAKE_MODULE_PATH ${BIOEXPLORER_SOURCE_DIRS}/CMake)

    find_package(CUDA REQUIRED)
    if(CUDA_FOUND)
      # This doesn't get called from common_find_package_post unless we export some
      # variables to the parent scope
      find_cuda_compatible_host_compiler()
    endif()
    message(STATUS "[BioExplorer] Using CUDA version ${CUDA_VERSION}")
    
    set(CUDA_NVCC_FLAGS "--use_fast_math")
    
    set(${NAME}_CU
        module/cuda/renderer/artistic/Golgi.cu
        module/cuda/renderer/Density.cu
        module/cuda/renderer/PathTracing.cu
        module/cuda/renderer/Voxel.cu
    )
    
    CUDA_WRAP_SRCS(${LIBRARY_NAME} PTX ptx_generated_files ${${NAME}_CU})
    
    include(StringifyPtx)
    stringify_ptx(${ptx_generated_files})

    list(APPEND ${NAME}_SOURCES
        ${${NAME}_CU}
        ${ptx_generated_files}
        ${PTX_SOURCES}
    )

    list(APPEND ${NAME}_HEADERS ${PTX_HEADERS})
    include_directories(${BIOEXPLORER_SOURCE_DIRS} ${OptiX6_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS})
    list(APPEND ${NAME}_PUBLIC_MODULE_LIBRARIES ${OptiX6_LIBRARIES})
    list(APPEND ${NAME}_PRIVATE_MODULE_LIBRARIES OptiX6Engine)
endif()

# ==============================================================================
# Compile c++ code
# ==============================================================================
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${OSPRAY_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/bioexplorer/backend
)

set(${NAME}_LINK_LIBRARIES
    PUBLIC ${${NAME}_PUBLIC_MODULE_LIBRARIES}
    PRIVATE CoreParameters CoreCommon CoreEngine CoreIO mpfr stdc++fs
    ${${NAME}_PRIVATE_MODULE_LIBRARIES} ${PQXX_LIBRARIES} ${OpenMP_CXX_LIBRARIES}
)

if(${CGAL_FOUND})
list(APPEND ${NAME}_LINK_LIBRARIES ${CGAL_LIBRARIES} gmp)
endif()

add_library(
    ${LIBRARY_NAME}
    SHARED ${${NAME}_SOURCES})

target_link_libraries(
    ${LIBRARY_NAME}
    ${${NAME}_LINK_LIBRARIES})

# ==============================================================================
# Unit testing
# ==============================================================================
option(${NAME}_UNIT_TESTING_ENABLED "Enable unit testing" ON)

if(${NAME}_UNIT_TESTING_ENABLED)
    add_subdirectory(tests)
endif()

# ==============================================================================
# Install binaries
# ==============================================================================
INSTALL(TARGETS ${LIBRARY_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

# ==============================================================================
# Plugins
# ==============================================================================
option(BIOEXPLORER_MEDIA_MAKER_ENABLED "Enable media maker plugin" ON)
if(BIOEXPLORER_MEDIA_MAKER_ENABLED)
    message(STATUS "[BioExplorer] Building MediaMaker plugin")
    add_subdirectory(plugins/MediaMaker)
endif()

option(BIOEXPLORER_METABOLISM_ENABLED "Enable metabolism plugin" ON)
if(BIOEXPLORER_METABOLISM_ENABLED)
    message(STATUS "[BioExplorer] Building Metabolism plugin")
    add_subdirectory(plugins/Metabolism)
endif()

option(BIOEXPLORER_SONATA_ENABLED "Enable SONATA plugin" OFF)
if(BIOEXPLORER_SONATA_ENABLED)
    message(STATUS "[BioExplorer] Building SONATA plugin")
    add_subdirectory(plugins/Sonata)
endif()

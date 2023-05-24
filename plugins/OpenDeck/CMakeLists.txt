# Copyright (c) 2015-2023, EPFL/Blue Brain Project
# All rights reserved. Do not distribute without permission.
#
# This file is part of Brayns <https://github.com/BlueBrain/Brayns>

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(LIBRARY_NAME braynsOpenDeck)
project(${LIBRARY_NAME} VERSION 1.0.0)
set(${LIBRARY_NAME}_VERSION_ABI 1)

include(Common)

common_find_package(ospray 1.8 SYSTEM)
common_find_package_post()

include_directories(
  ${CMAKE_BINARY_DIR}/generated/opendeck
)

set(BRAYNSOPENDECK_HEADERS
  plugin/common/OpenDeckParameters.h
  OpenDeckPlugin.h
)

set(
  BRAYNSOPENDECK_SOURCES
  plugin/common/OpenDeckParameters.cpp
  OpenDeckPlugin.cpp
)

set(
  BRAYNSOPENDECK_LINK_LIBRARIES
  PRIVATE braynsCommon braynsEngine braynsPluginAPI)

if(BRAYNS_OSPRAY_ENABLED)
  message(STATUS "[OpenDeck] OSPRay 1 module enabled")

  list(APPEND BRAYNSOPENDECK_SOURCES
    module/ispc/cylindric/CylindricCamera.cpp
    module/ispc/cylindricStereo/CylindricStereoCamera.cpp
    module/ispc/cylindricStereoTracked/CylindricStereoTrackedCamera.cpp)

  set(BRAYNSOPENDECK_ISPC_SOURCES
    module/ispc/cylindric/CylindricCamera.ispc
    module/ispc/cylindricStereo/CylindricStereoCamera.ispc
    module/ispc/cylindricStereoTracked/CylindricStereoTrackedCamera.ispc)

  list(APPEND BRAYNSOPENDECK_SOURCES ${BRAYNSOPENDECK_ISPC_SOURCES})

  # reuse ispc setup and macros from ospray
  list(APPEND CMAKE_MODULE_PATH ${OSPRAY_CMAKE_ROOT})
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(OSPRAY_DEBUG_BUILD ON)
  endif()
  include(ispc)

  # Compile ispc code
  include_directories_ispc(${PROJECT_SOURCE_DIR}/../../ ${OSPRAY_INCLUDE_DIRS})
  ospray_ispc_compile(${BRAYNSOPENDECK_ISPC_SOURCES})
  list(APPEND BRAYNSOPENDECK_SOURCES ${ISPC_OBJECTS})

  list(APPEND BRAYNSOPENDECK_LINK_LIBRARIES ospray::ospray_common ospray::ospray ospray::ospray_module_ispc)
endif()

# ==============================================================================
# OptiX 6 module
# ==============================================================================
if(BRAYNS_OPTIX6_ENABLED)
    list(APPEND CMAKE_MODULE_PATH ${BIOEXPLORER_SOURCE_DIR}/CMake)

    find_package(OptiX6 REQUIRED)
    find_package(CUDA REQUIRED)
    if(CUDA_FOUND)
      # This doesn't get called from common_find_package_post unless we export some
      # variables to the parent scope
      find_cuda_compatible_host_compiler()
    endif()
    message(STATUS "[OpenDeck] OptiX 6 module enabled")
    
    set(CUDA_NVCC_FLAGS "--use_fast_math")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shadow")
    
    set(${LIBRARY_NAME}_CU
        module/cuda/camera/cylindricStereo/CylindricStereoCamera.cu
    )

    list(APPEND BRAYNSOPENDECK_SOURCES
      plugin/optix6/OptiXCylindricStereoCamera.cpp
    )
    
    CUDA_WRAP_SRCS(${LIBRARY_NAME} PTX ptx_generated_files ${${LIBRARY_NAME}_CU})
    
    include(StringifyPtx)
    stringify_ptx(${ptx_generated_files})

    list(APPEND BRAYNSOPENDECK_SOURCES
        ${${LIBRARY_NAME}_CU}
        ${ptx_generated_files}
        ${PTX_SOURCES}
    )

    list(APPEND ${LIBRARY_NAME}_HEADERS ${PTX_HEADERS})

    include_directories(${OptiX6_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS})
    list(APPEND BRAYNSOPENDECK_LINK_LIBRARIES ${OptiX6_LIBRARIES} braynsOptix6Engine)
endif()

# ==============================================================================
# Compile c++ code
# ==============================================================================
configure_file(
    plugin/common/Defines.h.in
    ${CMAKE_BINARY_DIR}/generated/opendeck/Defines.h
)

set(BRAYNSOPENDECK_OMIT_LIBRARY_HEADER ON)
set(BRAYNSOPENDECK_OMIT_VERSION_HEADERS ON)
set(BRAYNSOPENDECK_OMIT_EXPORT ON)
set(BRAYNSOPENDECK_INCLUDE_NAME brayns_opendeck)
common_library(braynsOpenDeck)

if (TARGET Brayns-all)
  add_dependencies(Brayns-all braynsOpenDeck)
endif()

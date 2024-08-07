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

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(NAME BIOEXPLORER)

set(PACKAGE_VERSION_ABI 1)
set(PACKAGE_VERSION_MAJOR 2)
set(PACKAGE_VERSION_MINOR 0)
set(PACKAGE_VERSION_PATCH 0)

# Get the Git revision using the git command
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PACKAGE_VERSION_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
project(${NAME} VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_REV_STRING "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}.${PACKAGE_VERSION_REVISION}")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
set(${NAME}_MODULES_DIR ${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)

find_package(glm REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(ospray)
find_package(OptiX6)
find_package(CUDA)
find_package(assimp)
find_package(LibJpegTurbo)
find_package(Rockets)
find_package(CGAL)

set(BIOEXPLORER_SOURCE_DIRS ${PROJECT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated)

# ==============================================================================
# Options
# ==============================================================================

# Find the Intel C++ compiler
find_program(CMAKE_CXX_COMPILER NAMES icpc)
if(CMAKE_CXX_COMPILER)
    # If found, set the compiler and related flags
    message(STATUS "[Core] Building with Intel C++ compiler")
    # Set the compiler
    set(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER} CACHE FILEPATH "Intel C++ compiler" FORCE)
endif()

set(OSPRAY_FOUND 0)
if(ospray_FOUND)
    include(ispc)
    option(PLATFORM_OSPRAY_ENABLED "Enable OSPRay 1 module" ON)
    if(PLATFORM_OSPRAY_ENABLED)
        set(OSPRAY_FOUND 1)
        message(STATUS "[Core] OSPRay module enabled")
    endif()
endif()

set(OPTIX6_FOUND 0)
if(OptiX6_FOUND AND CUDA_FOUND)
    option(PLATFORM_OPTIX6_ENABLED "Enable OptiX 6 module" OFF)
    if(PLATFORM_OPTIX6_ENABLED)
        set(OPTIX6_FOUND 1)
        message(STATUS "[Core] OptiX 6 module enabled")
    endif()
endif()

if(CGAL_FOUND)
    if(${${NAME}_USE_CGAL})
        message(STATUS "[Core] CGAL module enabled")
        set(CGAL_DATA_DIR ".")
        find_package(CGAL REQUIRED)
    else()
        message(STATUS "[Core] CGAL module disabled")
    endif()
else()
    message(STATUS "[Core] CGAL module not found")
endif()

set(ASSIMP_FOUND 0)
if(assimp_FOUND)
    option(PLATFORM_ASSIMP_ENABLED "Activate assimp library for standard 3D file format management" ${assimp_FOUND})
    if(PLATFORM_ASSIMP_ENABLED)
        set(ASSIMP_FOUND 1)
        link_directories(${ASSIMP_LIBRARY_DIRS})
        message(STATUS "[Core] Assimp module enabled")
    endif()
endif()

set(NETWORKING_FOUND 0)
if(LibLIBJPEGTURBO_FOUND AND Rockets_FOUND)
    if(ROCKETS_USE_LIBWEBSOCKETS)
        option(PLATFORM_NETWORKING_ENABLED "Activate networking interfaces" ON)
    else()
        option(PLATFORM_NETWORKING_ENABLED "Activate networking interfaces" OFF)
    endif()

    if(PLATFORM_NETWORKING_ENABLED)
        message(STATUS "[Core] Building networking module")
        set(NETWORKING_FOUND 1)
        # We require turbojpeg for networking to get image streaming working

        if(NOT ROCKETS_USE_LIBWEBSOCKETS)
            message(WARNING "Could not enable networking, missing Rockets with libwebsockets")
        endif()

        list(APPEND COMMON_FIND_PACKAGE_DEFINES PLATFORM_USE_NETWORKING)
    endif()
endif()

# ==============================================================================
# Update Defines.h.in and Version.h.in
# ==============================================================================
configure_file(
    common/Defines.h.in
    ${CMAKE_BINARY_DIR}/generated/Defines.h
)

configure_file(
    common/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/Version.h
)

# ==============================================================================
# Core platform
# ==============================================================================
option(PLATFORM_BUILD_ENABLED "Activate platform build" ON)

if(PLATFORM_BUILD_ENABLED OR BIOEXPLORER_BUILD_ENABLED)
    add_subdirectory(common/deps)
endif()

if(PLATFORM_BUILD_ENABLED)
    message(STATUS "[Core] Platform build enabled")
    add_subdirectory(platform)
endif()

# ==============================================================================
# BioExplorer backend
# ==============================================================================
option(BIOEXPLORER_BUILD_ENABLED "Activate BioExplorer build" ON)

if(BIOEXPLORER_BUILD_ENABLED)
    message(STATUS "[Core] BioExplorer build enabled")
    add_subdirectory(bioexplorer/backend)
endif()

# ==============================================================================
# Space explorer
# ==============================================================================
option(COSMOLOGY_BUILD_ENABLED "Activate cosmology extensions build" OFF)

if(COSMOLOGY_BUILD_ENABLED)
    message(STATUS "[Core] Cosmology build enabled")
    add_subdirectory(extensions/cosmology)
endif()

# ==============================================================================
# Medical imaging explorer
# ==============================================================================
option(MEDICALIMAGING_BUILD_ENABLED "Activate medical imaging extensions build" OFF)

if(MEDICALIMAGING_BUILD_ENABLED)
    message(STATUS "[Core] Medical imaging build enabled")
    add_subdirectory(extensions/medicalimaging)
endif()

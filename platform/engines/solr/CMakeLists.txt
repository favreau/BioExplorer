# Copyright (c) 2018, EPFL/Blue Brain Project
# All rights reserved. Do not distribute without permission.
# Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
#
# This file is part of Brayns <https://github.com/BlueBrain/Brayns>

# The Blue Brain BioExplorer is a tool for scientists to extract and analyse
# scientific data from visualization
#
# Copyright 2020-2023 Blue BrainProject / EPFL
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <https://www.gnu.org/licenses/>.

# ==============================================================================
# Project
# ==============================================================================
set(NAME SOLR_ENGINE)
set(LIBRARY_NAME SolREngine)

find_package(SolR REQUIRED)

set(${NAME}_SOURCES
  SolRContext.cpp
  SolRCamera.cpp
  SolREngine.cpp
  SolRFrameBuffer.cpp
  SolRMaterial.cpp
  SolRModel.cpp
  SolRRenderer.cpp
  SolRScene.cpp
  SolRVolume.cpp
)

set(${NAME}
  SolRContext.h
  SolRCamera.h
  SolREngine.h
  SolRFrameBuffer.h
  SolRMaterial.h
  SolRModel.h
  SolRRenderer.h
  SolRScene.h
  SolRVolume.h
)

# TODO (Add lib according to engine: Cuda or OpenCL)
set(${NAME}_LINK_LIBRARIES
    PRIVATE CoreCommon CoreParameters ${SOLR_LIBRARY} ${CUDA_LIBRARIES}
)

# ==============================================================================
# Compile c++ code
# ==============================================================================
include_directories(
  ${BIOEXPLORER_SOURCE_DIRS}
  ${CMAKE_BINARY_DIR}
  ${SOLR_INCLUDE_DIRS}
  ${CUDA_INCLUDE_DIRS}
)

add_library(
    ${LIBRARY_NAME}
    SHARED ${${NAME}_SOURCES})


if(CUDA_VERSION VERSION_GREATER 8)
  target_compile_options(${LIBRARY_NAME} PRIVATE "-Wno-error=cpp")
endif()

target_link_libraries(
    ${LIBRARY_NAME}
    ${${NAME}_LINK_LIBRARIES}
)

# ==============================================================================
# Install binaries
# ==============================================================================
install(
  TARGETS ${LIBRARY_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

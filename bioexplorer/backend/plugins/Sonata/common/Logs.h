/*
 * Copyright 2020-2024 Blue Brain Project / EPFL
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

#include <platform/core/common/Timer.h>

#include <iostream>

namespace sonataexplorer
{
namespace common
{
#define PLUGIN_PREFIX "SONATA_EXPLORER "
#define PROGRESS_BAR_SIZE 50

#define PLUGIN_ERROR(__msg) std::cerr << "E [" << PLUGIN_PREFIX << "] " << __msg << std::endl;
#define PLUGIN_WARN(__msg) std::cerr << "W [" << PLUGIN_PREFIX << "] " << __msg << std::endl;
#define PLUGIN_INFO(__msg) std::cout << "I [" << PLUGIN_PREFIX << "] " << __msg << std::endl;

#ifdef NDEBUG
#define PLUGIN_DEBUG(__msg) ;
#else
#define PLUGIN_DEBUG(__msg)                                                \
    {                                                                      \
        std::cout << "D [" << PLUGIN_PREFIX << "] " << __msg << std::endl; \
    }
#endif
#define PLUGIN_TIMER(__time, __msg) \
    std::cout << "T [" << PLUGIN_PREFIX << "] [" << __time << "] " << __msg << std::endl;

#define PLUGIN_THROW(__msg)              \
    {                                    \
        throw std::runtime_error(__msg); \
    }

#define PLUGIN_PROGRESS(__msg, __progress, __maxValue)                                                      \
    {                                                                                                       \
        std::cout << "I [" << PLUGIN_PREFIX << "] [";                                                       \
        const float __mv = float(__maxValue);                                                               \
        const float __p = float(__progress + 1);                                                            \
        const uint32_t __pos = std::min(PROGRESS_BAR_SIZE, int(__p / __mv * PROGRESS_BAR_SIZE));            \
        for (uint32_t __i = 0; __i < PROGRESS_BAR_SIZE; ++__i)                                              \
        {                                                                                                   \
            if (__i < __pos)                                                                                \
                std::cout << "=";                                                                           \
            else if (__i == __pos)                                                                          \
                std::cout << ">";                                                                           \
            else                                                                                            \
                std::cout << " ";                                                                           \
        }                                                                                                   \
        std::cout << "] " << std::min(__pos * 2, uint32_t(PROGRESS_BAR_SIZE * 2)) << "% " << __msg << "\r"; \
        std::cout.flush();                                                                                  \
    }

} // namespace common
} // namespace sonataexplorer

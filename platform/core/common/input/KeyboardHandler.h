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

#include <functional>

namespace core
{
struct ShortcutInformation
{
    std::string description;
    std::function<void()> functor;
};

enum class SpecialKey
{
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class KeyboardHandler
{
public:
    void registerKeyboardShortcut(const unsigned char key, const std::string& description,
                                  std::function<void()> functor);

    void unregisterKeyboardShortcut(const unsigned char key);

    void handleKeyboardShortcut(const unsigned char key);

    void registerSpecialKey(const SpecialKey key, const std::string& description, std::function<void()> functor);

    void unregisterSpecialKey(const SpecialKey key);

    void handle(const SpecialKey key);

    const std::vector<std::string>& help() const;

    const std::string getKeyboardShortcutDescription(const unsigned char key);

private:
    void _buildHelp();

    std::map<unsigned char, ShortcutInformation> _registeredShortcuts;
    std::map<SpecialKey, ShortcutInformation> _registeredSpecialKeys;
    std::vector<std::string> _helpStrings;
};
} // namespace core

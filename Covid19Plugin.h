/* Copyright (c) 2020, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
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

#ifndef COVID19_PLUGIN_H
#define COVID19_PLUGIN_H

#include <api/Covid19Params.h>

#include <brayns/pluginapi/ExtensionPlugin.h>

/**
 * @brief This class implements the visualization of Covid19 related resources
 */
class Covid19Plugin : public brayns::ExtensionPlugin
{
public:
    Covid19Plugin();

    void init() final;

private:
    Response _addAssembly(const AssemblyDescriptor &payload);
    Response _removeAssembly(const AssemblyDescriptor &payload);
    Response _addRNASequence(const RNASequenceDescriptor &payload);
    Response _addProtein(const ProteinDescriptor &payload);
    Response _addMesh(const MeshDescriptor &payload);
    Response _addGlycans(const GlycansDescriptor &payload);

    Response _setColorScheme(const ColorSchemeDescriptor &payload);
    Response _setAminoAcidSequenceAsString(
        const AminoAcidSequenceAsStringDescriptor &payload);
    Response _setAminoAcidSequenceAsRange(
        const AminoAcidSequenceAsRangeDescriptor &payload);
    Response _getAminoAcidSequences(
        const AminoAcidSequencesDescriptor &payload);

    AssemblyMap _assemblies;
};

#endif

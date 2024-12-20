# Copyright 2020 - 2023 Blue Brain Project / EPFL
#
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

from bioexplorer import (
    BioExplorer,
    Cell,
    Membrane,
    Protein,
    Vector2,
    Vector3,
    Quaternion,
)
import os

# pylint: disable=no-member
# pylint: disable=missing-function-docstring
# pylint: disable=dangerous-default-value


def test_cell():
    name = "Cell"
    resource_folder = os.path.abspath("./tests/test_files")
    pdb_folder = os.path.join(resource_folder, "pdb")
    membrane_folder = os.path.join(pdb_folder, "membrane")

    bio_explorer = BioExplorer("localhost:5000")
    bio_explorer.reset_scene()
    bio_explorer.start_model_loading_transaction()

    # Suspend image streaming
    bio_explorer.core_api().set_application_parameters(image_stream_fps=0)

    # Proteins
    protein_representation = bio_explorer.protein_representation.ATOMS

    # Membrane parameters
    membrane_size = Vector3(800.0, 80.0, 800.0)
    membrane_nb_receptors = 20

    # ACE2 Receptor
    ace2_receptor = Protein(
        name=name + "_" + bio_explorer.NAME_RECEPTOR,
        source=os.path.join(pdb_folder, "6m1d.pdb"),
        occurrences=membrane_nb_receptors,
        transmembrane_params=Vector2(-6.0, 5.0),
    )

    membrane = Membrane(lipid_sources=[os.path.join(membrane_folder, "popc.pdb")])

    cell = Cell(
        name=name,
        shape=bio_explorer.assembly_shape.SINUSOID,
        shape_params=membrane_size,
        membrane=membrane,
        proteins=[ace2_receptor],
    )

    bio_explorer.add_cell(
        cell=cell,
        position=Vector3(4.5, -186, 7.0),
        rotation=Quaternion(1, 0, 0, 0),
        representation=protein_representation,
    )

    # Set rendering settings
    bio_explorer.core_api().set_renderer(
        background_color=[96 / 255, 125 / 255, 139 / 255],
        current="advanced",
        samples_per_pixel=1,
        subsampling=4,
        max_accum_frames=64,
    )
    params = bio_explorer.core_api().AdvancedRendererParams()
    params.shadow_intensity = 0.75
    params.soft_shadow_strength = 1.0
    bio_explorer.core_api().set_renderer_params(params)

    # Restore image streaming
    bio_explorer.core_api().set_application_parameters(image_stream_fps=20)


if __name__ == "__main__":
    import nose

    nose.run(defaultTest=__name__)

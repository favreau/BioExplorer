# !/usr/bin/env python
"""BioExplorer class"""

# -*- coding: utf-8 -*-

# The Blue Brain BioExplorer is a tool for scientists to extract and analyse
# scientific data from visualization
#
# Copyright 2020-2022 Blue Brain Project / EPFL
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

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from sqlalchemy import create_engine
from ipywidgets import Layout, GridspecLayout, Select, ColorPicker, FloatSlider


class Metabolism:
    """
    The Metabolism class defines an API to access and render metabolism information.

    Metabolism data is stored in a PostgreSQL database.
    """

    PLUGIN_API_PREFIX = 'mb-'

    def __init__(
            self, bioexplorer, model_id, db_host, db_name, db_user, db_password,
            db_schema, simulation_timestamp):
        """
        Metabolism class initialization

        :bioexplorer: Reference to a instance of the BioExplorer
        :model_id: Id of the model
        :db_host: Host name of the PostgreSQL server
        :db_host: Database name
        :db_host: Database user name
        :db_host: Database password
        :db_schema: Database scheme
        :simulation_timestamp: Simulation timestamp
        """
        self._be = bioexplorer
        self._core = self._be.core_api()
        self._db_host = db_host
        self._db_name = db_name
        self._db_user = db_user
        self._db_password = db_password
        self._db_schema = db_schema

        db_connection_string = 'postgresql+psycopg2://%s:%s@%s:5432/%s' % (
            db_user, db_password, db_host, db_name)
        self._engine = create_engine(db_connection_string)
        self._db_connection = self._engine.connect()

        self._simulation_guid = self._get_simulation_guid(simulation_timestamp)
        self._metabolite_ids = dict()
        self._model_id = model_id

    def _get_simulation_guid(self, timestamp):
        try:
            sql_command = "SELECT guid FROM %s.simulation WHERE timestamp='%s'" % (
                self._db_schema, timestamp)
            data = pd.read_sql(sql_command, self._db_connection)
            return int(data['guid'])
        except Exception as e:
            print(e)

    def set_renderer(
            self, max_accum_frames=None, subsampling=None, alpha_correction=0.1,
            exposure=1.0, near_plane=10.0, far_plane=200.0, ray_step=3.0,
            search_length=15.0, noise_frequency=1.0, noise_amplitude=1.0,
            use_random_search=False):
        """
        Set the metabolism renderer.

        The renderer travels through the scene using
        the ray-marching technique. For every voxel, the region is identified and
        the metabolite concentration is used to define the opacity of the voxel.

        :max_accum_frames: Maximum number of accumulated framed
        :subsampling: Sub-sampling
        :alpha_correction: Alpha correction applied to voxels
        :exposure: Exposure applied on the final rendering
        :near_plane: Volume near plane
        :far_plane: Volume far plane
        :ray_step: Step for the ray marching process
        :search_length: Length of the ray used to search the closest surface
        :noise_frequency: Noise frequency for the cloud rendering effect
        :noise_amplitude: Noise amplitude for the cloud rendering effect
        :use_random_search: Uses random directions for the search ray if enabled
        """
        self._core.set_renderer(
            current='metabolism', subsampling=subsampling,
            max_accum_frames=max_accum_frames)
        params = self._core.MetabolismRendererParams()
        params.alpha_correction = alpha_correction
        params.exposure = exposure
        params.near_plane = near_plane
        params.far_plane = far_plane
        params.ray_step = ray_step
        params.refinement_steps = max_accum_frames
        params.search_length = search_length
        params.noise_frequency = noise_frequency
        params.noise_amplitude = noise_amplitude
        params.use_random_search = use_random_search
        self._core.set_renderer_params(params)

    def _put_metabolite_ids(self):
        ids = list()
        for metabolite_id in self._metabolite_ids:
            ids.append(int(self._metabolite_ids[metabolite_id]))

        db_connection_string = 'host=%s port=5432 dbname=%s user=%s password=%s' % (
            self._db_host, self._db_name, self._db_user, self._db_password)

        params = dict()
        params['connectionString'] = db_connection_string
        params['schema'] = self._db_schema
        params['simulationId'] = self._simulation_guid
        params['metaboliteIds'] = ids
        result = self._core.rockets_client.request(
            method=self.PLUGIN_API_PREFIX + 'attach-handler', params=params)
        return result

    def callback(self, location, metabolite_id):
        """
        Call back function used by the Metabolism widget to update the rendering

        :location: Location (Neuron cytosol, Astrocyte mitochondrion, etc)
        :metabolite_id: Metabolite id
        """
        self._metabolite_ids[location] = metabolite_id
        self._put_metabolite_ids()
        self._core.set_renderer()

    def display(self):
        """
        Displays a widget in the notebook.

        The widget allows selection of metabolites and regions that should be rendered
        """
        class Updated:
            def __init__(
                    self, client, model_id, db_connection, db_schema, simulation_guid,
                    callback):
                self._core = client
                self._model_id = model_id
                self._db_connection = db_connection
                self._db_schema = db_schema
                self._simulation_guid = simulation_guid
                self._location = None
                self._grid = None
                self._metabolite_slider = None
                self._metabolite_selector = None
                self._callback = callback
                self._metabolites = dict()
                self._locations = dict()
                self._selections = dict()
                self._location_colors = dict()
                self._location_colors_opacity = dict()
                self.get_location_colors()

            def _html_color(self, rgb_color):
                color_as_string = '#' + \
                    '%02x' % (int)(rgb_color[0] * 255) + \
                    '%02x' % (int)(rgb_color[1] * 255) + \
                    '%02x' % (int)(rgb_color[2] * 255)
                return color_as_string

            @ staticmethod
            def _hex_to_rgb(value):
                value = value.lstrip('#')
                lv = len(value)
                return tuple(int(value[i:i + lv // 3], 16) for i in range(0, lv, lv // 3))

            def _get_data(self, guid):
                sql = "SELECT 100 * c.concentration / v.base_value as concentration " \
                    "FROM %s.variable as v, %s.concentration as c " \
                    "WHERE v.guid=c.variable_guid AND " \
                    "v.guid=%d AND c.simulation_guid=%d" % (
                        self._db_schema, self._db_schema, guid, self._simulation_guid)
                data = pd.read_sql(sql, self._db_connection)
                values = list()
                for i in range(len(data)):
                    values.append(float(data['concentration'][i]))
                return np.array(values, np.float32)

            def get_locations(self):
                sql = "SELECT guid, description FROM %s.location ORDER BY description" % (
                    self._db_schema)
                data = pd.read_sql(sql, self._db_connection)
                for i in range(len(data)):
                    if i == 0:
                        self._location = data['guid'][i]
                    self._locations[data['description'][i]] = data['guid'][i]
                return self._locations

            def get_location_colors(self):
                sql = "SELECT guid, red, green, blue FROM %s.location ORDER BY guid" \
                    % self._db_schema
                data = pd.read_sql(sql, self._db_connection)
                for i in range(len(data)):
                    guid = int(data['guid'][i])
                    color = self._html_color(
                        [data['red'][i], data['green'][i], data['blue'][i]])
                    self._location_colors[guid] = color
                    self._location_colors_opacity[guid] = 0.5

            def get_metabolites(self):
                self._metabolites = dict()
                sql = "SELECT v.guid, v.description FROM %s.variable AS v, %s.concentration AS c " \
                    "WHERE v.location_guid=%d AND v.unit_guid=0 AND v.guid=c.variable_guid AND " \
                    "c.simulation_guid=%d " \
                    "ORDER BY description" % (self._db_schema, self._db_schema,
                                              self._location, self._simulation_guid)
                data = pd.read_sql(sql, self._db_connection)
                for i in range(len(data)):
                    self._metabolites[data['description'][i]] = data['guid'][i]
                return self._metabolites

            def update_metabolite(self, change):
                if change.new:
                    y = self._get_data(change.new)
                    x = np.linspace(0, len(y), len(y))
                    line.set_xdata(x)
                    line.set_ydata(y)
                    ax.axes.set_xlabel('Seconds')
                    ax.axes.set_ylabel('mM')
                    ax.axes.relim()
                    ax.axes.autoscale_view()
                    figure.canvas.draw()
                    self._callback(self._location, change.new)

            def update_location(self, change):
                self._selections[self._location] = metabolite_selector.value
                self._location = change.new
                location_color_picker.value = self._location_colors[self._location]
                location_color_opacity_slider.value = self._location_colors_opacity[
                    self._location]
                metabolite_selector.options = self.get_metabolites()
                if self._location in self._selections:
                    metabolite_selector.value = self._selections[self._location]

            def update_location_color(self, change):
                self._location_colors[self._location] = change.new
                self._update_palette()

            def update_location_color_opacity(self, change):
                self._location_colors_opacity[self._location] = change.new
                self._update_palette()

            def _update_palette(self):
                btf = self._core.get_model_transfer_function(
                    id=self._model_id)
                colors = list()
                points = list()
                nb_points = len(self._locations)
                step = 1.0 / float(nb_points - 1)
                for i in range(nb_points):
                    color = self._hex_to_rgb(self._location_colors[i])
                    colors.append([
                        float(color[0]) / 256.0,
                        float(color[1]) / 256.0,
                        float(color[2]) / 256.0])
                    points.append([i * step, self._location_colors_opacity[i]])

                btf['colormap']['name'] = 'TransferFunctionEditor'
                btf['colormap']['colors'] = colors
                btf['opacity_curve'] = points
                btf['range'] = [0, nb_points - 1]
                self._core.set_model_transfer_function(
                    id=self._model_id, transfer_function=btf)

        def update_location(value):
            update_class.update_location(value)

        def update_location_color(value):
            update_class.update_location_color(value)

        def update_location_color_opacity(value):
            update_class.update_location_color_opacity(value)

        def update_metabolite(value):
            update_class.update_metabolite(value)

        update_class = Updated(self._core, self._model_id, self._db_connection,
                               self._db_schema, self._simulation_guid, self.callback)

        y = np.linspace(0, 1, 1)
        x = np.linspace(0, 1, 1)
        figure, ax = plt.subplots(figsize=(7.5, 2))
        line, = ax.plot(x, y)
        ax.grid(True)
        grid = GridspecLayout(n_rows=3, n_columns=2, height='170px')
        location_selector = Select(
            options=update_class.get_locations(), description='Location',
            layout=Layout(height="100px", width="350px"))
        metabolite_selector = Select(
            options=update_class.get_metabolites(), description='Metabolite',
            layout=Layout(height="100px", width="350px"))
        location_color_opacity_slider = FloatSlider(
            value=1.0, min=0.0, max=1.0, step=0.1, description='Opacity',
            layout=Layout(width="350px"))
        location_color_picker = ColorPicker(
            description='Color', layout=Layout(width="350px"))

        grid[0, 0] = location_selector
        grid[1, 0] = location_color_picker
        grid[2, 0] = location_color_opacity_slider
        grid[0, 1] = metabolite_selector
        location_selector.observe(update_location, 'value')
        metabolite_selector.observe(update_metabolite, 'value')
        location_color_picker.observe(update_location_color, 'value')
        location_color_opacity_slider.observe(
            update_location_color_opacity, 'value')
        display(grid)
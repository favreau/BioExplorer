#!/usr/bin/env

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

"""
Module movie_maker

This module provides SDK for the MediaMaker plug-in.
"""


import copy
import time
import pyexiv2
from ipywidgets import IntSlider, IntProgress
from IPython.display import display
from .bio_explorer import BioExplorer
from .version import VERSION as __version__

# pylint: disable=no-member
# pylint: disable=dangerous-default-value
# pylint: disable=too-many-arguments
# pylint: disable=too-many-locals


class MovieMaker:
    """Movie maker"""

    PLUGIN_API_PREFIX = "mm-"
    FRAME_BUFFER_MODE_COLOR = 0
    FRAME_BUFFER_MODE_DEPTH = 1

    def __init__(self, bioexplorer):
        """
        Initialize the MovieMaker object

        :bioexplorer: BioExplorer client
        """
        assert isinstance(bioexplorer, BioExplorer)
        self._client = bioexplorer.core_api()
        self._smoothed_key_frames = list()

    @staticmethod
    def version():
        """
        Get the version of the SDK

        :return: The version of the SDK
        :rtype: string
        """
        return __version__

    def build_camera_path(
        self, control_points, nb_steps_between_control_points, smoothing_size=1
    ):
        """
        Build a camera path from control points

        :control_points: List of control points
        :nb_steps_between_control_points: Number of steps between two control points
        :smoothing_size: Number of steps to be considered for the smoothing of the path
        """
        origins = list()
        directions = list()
        ups = list()
        aperture_radii = list()
        focal_distances = list()
        self._smoothed_key_frames.clear()

        for s in range(len(control_points) - 1):
            p0 = control_points[s]
            p1 = control_points[s + 1]

            for i in range(nb_steps_between_control_points):
                origin = [0, 0, 0]
                direction = [0, 0, 0]
                up = [0, 0, 0]

                t_origin = [0, 0, 0]
                t_direction = [0, 0, 0]
                t_up = [0, 0, 0]
                for k in range(3):
                    t_origin[k] = (p1["origin"][k] - p0["origin"][k]) / float(
                        nb_steps_between_control_points
                    )
                    t_direction[k] = (p1["direction"][k] - p0["direction"][k]) / float(
                        nb_steps_between_control_points
                    )
                    t_up[k] = (p1["up"][k] - p0["up"][k]) / float(
                        nb_steps_between_control_points
                    )

                    origin[k] = p0["origin"][k] + t_origin[k] * float(i)
                    direction[k] = p0["direction"][k] + t_direction[k] * float(i)
                    up[k] = p0["up"][k] + t_up[k] * float(i)

                t_aperture_radius = (
                    p1["apertureRadius"] - p0["apertureRadius"]
                ) / float(nb_steps_between_control_points)
                aperture_radius = p0["apertureRadius"] + t_aperture_radius * float(i)

                t_focal_distance = (p1["focalDistance"] - p0["focalDistance"]) / float(
                    nb_steps_between_control_points
                )
                focal_distance = p0["focalDistance"] + t_focal_distance * float(i)

                origins.append(origin)
                directions.append(direction)
                ups.append(up)
                aperture_radii.append(aperture_radius)
                focal_distances.append(focal_distance)

        nb_frames = len(origins)
        for i in range(nb_frames):
            o = [0, 0, 0]
            d = [0, 0, 0]
            u = [0, 0, 0]
            aperture_radius = 0.0
            focal_distance = 0.0
            for j in range(int(smoothing_size)):
                index = int(max(0, min(i + j - smoothing_size / 2, nb_frames - 1)))
                for k in range(3):
                    o[k] = o[k] + origins[index][k]
                    d[k] = d[k] + directions[index][k]
                    u[k] = u[k] + ups[index][k]
                aperture_radius = aperture_radius + aperture_radii[index]
                focal_distance = focal_distance + focal_distances[index]
            self._smoothed_key_frames.append(
                [
                    (
                        o[0] / smoothing_size,
                        o[1] / smoothing_size,
                        o[2] / smoothing_size,
                    ),
                    (
                        d[0] / smoothing_size,
                        d[1] / smoothing_size,
                        d[2] / smoothing_size,
                    ),
                    (
                        u[0] / smoothing_size,
                        u[1] / smoothing_size,
                        u[2] / smoothing_size,
                    ),
                    aperture_radius / smoothing_size,
                    focal_distance / smoothing_size,
                ]
            )
        last = control_points[len(control_points) - 1]
        self._smoothed_key_frames.append(
            (
                last["origin"],
                last["direction"],
                last["up"],
                last["apertureRadius"],
                last["focalDistance"],
            )
        )

    def get_nb_frames(self):
        """
        Get the number of smoothed frames

        :return: The number of smoothed frames
        :rtype: integer
        """
        return len(self._smoothed_key_frames)

    def get_key_frame(self, frame):
        """
        Get the smoothed camera information for the given frame

        :frame: Frame number
        :return: The smoothed camera information for the given frame
        :rtype: list
        :raises KeyError: if the frame is out of range
        """
        if frame < len(self._smoothed_key_frames):
            return self._smoothed_key_frames[frame]
        raise KeyError

    def set_camera(self, origin, direction, up):
        """
        Set the camera using origin, direction and up vectors

        :origin: Origin of the camera
        :direction: Direction in which the camera is looking
        :up: Up vector
        :return: Result of the request submission
        :rtype: Response
        """
        params = dict()
        params["origin"] = origin
        params["direction"] = direction
        params["up"] = up
        return self._client.rockets_client.request(
            self.PLUGIN_API_PREFIX + "set-odu-camera", params
        )

    def get_camera(self):
        """
        Get the origin, direction and up vector of the camera

        :return: A JSon representation of the origin, direction and up vectors
        :rtype: Response
        """
        return self._client.rockets_client.request(
            self.PLUGIN_API_PREFIX + "get-odu-camera"
        )

    def attach_camera_handler(
        self, control_points, nb_steps_between_control_points, smoothing_size=1
    ):
        """
        Attach a camera handler with specified control points and parameters.

        This method configures a camera handler using a series of control points and associated
        parameters to create a smooth camera path.

        :param control_points: list of dict: A list of dictionaries where each dictionary represents
        a control point with the following keys:
                            - "origin": A list of 3 float values representing the camera's origin.
                            - "direction": A list of 3 float values representing the camera's
                            direction.
                            - "up": A list of 3 float values representing the camera's up vector.
                            - "apertureRadius": A float value representing the aperture radius.
                            - "focalDistance": A float value representing the focal distance.
        :param nb_steps_between_control_points: int: The number of steps between each control point
        to ensure smooth transitions.
        :param smoothing_size: int, optional: The number of smoothing steps to apply (default is 1).

        :return: Response from the client's request to attach the camera handler.
        :rtype: Response object
        """
        origins = list()
        directions = list()
        ups = list()
        aperture_radii = list()
        focal_distances = list()
        for control_point in control_points:
            for k in range(3):
                origins.append(float(control_point["origin"][k]))
                directions.append(float(control_point["direction"][k]))
                ups.append(float(control_point["up"][k]))
            aperture_radii.append(float(control_point['apertureRadius']))
            focal_distances.append(float(control_point['focalDistance']))

        params = dict()
        params["origins"] = origins
        params["directions"] = directions
        params["ups"] = ups
        params["apertureRadii"] = aperture_radii
        params["focalDistances"] = focal_distances
        params["stepsBetweenKeyFrames"] = nb_steps_between_control_points
        params["numberOfSmoothingSteps"] = smoothing_size
        return self._client.rockets_client.request(
            self.PLUGIN_API_PREFIX + "attach-odu-camera-handler", params
        )

    def export_frames(
        self,
        size,
        path,
        base_name,
        image_format="png",
        animation_frames=list(),
        quality=100,
        samples_per_pixel=1,
        start_frame=0,
        end_frame=0,
        interpupillary_distance=0.0,
        export_intermediate_frames=False,
        frame_buffer_mode=FRAME_BUFFER_MODE_COLOR,
        keywords=list(),
    ):
        """
        Exports frames to disk. Frames are named using a 6 digit representation of the frame number

        :path: Folder into which frames are exported
        :image_format: Image format (the ones supported par Brayns: PNG, JPEG, etc)
        :quality: Quality of the exported image (Between 0 and 100)
        :samples_per_pixel: Number of samples per pixels
        :start_frame: Optional value if the rendering should start at a specific frame.
        :end_frame: Optional value if the rendering should end at a specific frame.
        :export_intermediate_frames: Exports intermediate frames (for every sample per pixel)
        :interpupillary_distance: Distance between pupils
        from zero. This is used to resume the rendering of a previously canceled sequence)
        :return: Result of the request submission
        :rtype: Response
        """
        nb_frames = self.get_nb_frames()
        if end_frame == 0:
            end_frame = nb_frames

        assert isinstance(size, list)
        assert len(size) == 2
        if len(animation_frames) != 0:
            assert len(animation_frames) == nb_frames
        assert start_frame <= end_frame
        assert end_frame <= nb_frames

        self._client.set_renderer(
            accumulation=True,
            samples_per_pixel=1,
            max_accum_frames=samples_per_pixel + 1,
            subsampling=1,
        )

        camera_definitions = list()
        for i in range(start_frame, end_frame):
            camera_definitions.append(self.get_key_frame(i))

        assert isinstance(keywords, list)
        keywords_as_string = ""
        for keyword in keywords:
            if keywords_as_string != "":
                keywords_as_string += ","
            keywords_as_string += keyword

        params = dict()
        params["path"] = path
        params["baseName"] = base_name
        params["format"] = image_format
        params["size"] = size
        params["quality"] = quality
        params["spp"] = samples_per_pixel
        params["startFrame"] = start_frame
        params["endFrame"] = end_frame
        params["exportIntermediateFrames"] = export_intermediate_frames
        params["animationInformation"] = animation_frames
        params["frameBufferMode"] = frame_buffer_mode
        params["keywords"] = keywords_as_string
        values = list()
        for camera_definition in camera_definitions:
            # Origin
            for i in range(3):
                values.append(camera_definition[0][i])
            # Direction
            for i in range(3):
                values.append(camera_definition[1][i])
            # Up
            for i in range(3):
                values.append(camera_definition[2][i])
            # Aperture radius
            values.append(camera_definition[3])
            # Focus distance
            values.append(camera_definition[4])
            # Interpupillary distance
            values.append(interpupillary_distance)

        params["cameraInformation"] = values
        return self._client.rockets_client.request(
            self.PLUGIN_API_PREFIX + "export-frames-to-disk", params
        )

    def get_export_frames_progress(self):
        """
        Queries the progress of the last export of frames to disk request

        :return: Dictionary with the result: "frameNumber" with the number of
        the last written-to-disk frame, and "done", a boolean flag stating wether
        the exporting is finished or is still in progress
        :rtype: Response
        """
        return self._client.rockets_client.request(
            self.PLUGIN_API_PREFIX + "get-export-frames-progress"
        )

    def cancel_frames_export(self):
        """
        Cancel the exports of frames to disk

        :return: Result of the request submission
        :rtype: Response
        """
        params = dict()
        params["path"] = "/tmp"
        params["baseName"] = ""
        params["format"] = "png"
        params["size"] = [64, 64]
        params["quality"] = 100
        params["spp"] = 1
        params["startFrame"] = 0
        params["endFrame"] = 0
        params["exportIntermediateFrames"] = False
        params["animationInformation"] = []
        params["cameraInformation"] = []
        params["frameBufferMode"] = MovieMaker.FRAME_BUFFER_MODE_COLOR
        params["keywords"] = ""
        return self._client.rockets_client.request(
            self.PLUGIN_API_PREFIX + "export-frames-to-disk", params
        )

    def set_current_frame(self, frame, camera_params=None):
        """
        Set the current animation frame

        :frame: Frame number
        :camera_params: Camera parameters. Defaults to None.
        """
        assert frame >= 0
        assert frame < self.get_nb_frames()

        cam = self.get_key_frame(frame)

        origin = list(cam[0])
        direction = list(cam[1])
        up = list(cam[2])

        self.set_camera(origin=origin, direction=direction, up=up)
        self._client.set_animation_parameters(current=frame)

        if camera_params is not None:
            camera_params.aperture_radius = cam[3]
            camera_params.focal_distance = cam[4]
            camera_params.enable_clipping_planes = False
            self._client.set_camera_params(camera_params)

    def display(self):
        """Displays a widget giving access to the movie frames"""
        frame = IntSlider(description="frame", min=0, max=self.get_nb_frames() - 1)

        def update_frame(args):
            frame.value = args["new"]
            self.set_current_frame(frame.value)

        frame.observe(update_frame, "value")
        display(frame)

    def _set_renderer_params(self, name, samples_per_pixel, gi_length=5.0):
        """
        Set renderer with default parameters

        :name: (string): Name of the renderer
        :gi_length: (float, optional): Max length of global illumination rays. Defaults to 5.0.

        :return: Frame buffer mode (color or depth)
        :rtype: int
        """
        spp = samples_per_pixel
        frame_buffer_mode = MovieMaker.FRAME_BUFFER_MODE_COLOR
        if name == "ambient_occlusion":
            params = self._client.AmbientOcclusionRendererParams()
            params.samples_per_frame = 16
            params.ray_length = gi_length
            self._client.set_renderer_params(params)
            spp = 4
        elif name == "depth":
            frame_buffer_mode = MovieMaker.FRAME_BUFFER_MODE_DEPTH
            spp = 1
        elif name in ["raycast_Ns", "radiance"]:
            spp = 4
        elif name == "shadow":
            params = self._client.ShadowRendererParams()
            params.samples_per_frame = 16
            params.ray_length = gi_length
            self._client.set_renderer_params(params)
            spp = 4
        return frame_buffer_mode, spp

    def create_snapshot(
        self,
        renderer,
        size,
        path,
        base_name,
        samples_per_pixel,
        export_intermediate_frames=False,
        gi_length=1e6,
        show_progress=False,
        keywords=list(),
    ):
        """
        Create a snapshot of the current frame

        :renderer: Name of the renderer
        :size: Frame buffer size
        :path: Path where the snapshot file is exported
        :base_name: Base name of the snapshot file
        :samples_per_pixel: Samples per pixel
        :export_intermediate_frames: If True, intermediate samples are stored to disk. Otherwise,
        only the final accumulation is exported
        gi_length (float, optional): Max length of global illumination rays. Defaults to 5.0.
        :keywords: List of keywords that will be added to the Xmp.dc.Subject tag
        """
        assert isinstance(size, list)
        assert isinstance(samples_per_pixel, int)
        assert len(size) == 2
        assert isinstance(export_intermediate_frames, bool)
        assert isinstance(gi_length, float)

        application_params = self._client.get_application_parameters()
        renderer_params = self._client.get_renderer()
        old_image_stream_fps = application_params["image_stream_fps"]
        old_viewport_size = application_params["viewport"]
        old_samples_per_pixel = renderer_params["samples_per_pixel"]
        old_max_accum_frames = renderer_params["max_accum_frames"]
        old_smoothed_key_frames = copy.deepcopy(self._smoothed_key_frames)

        self._client.set_renderer(
            current=renderer, samples_per_pixel=1, max_accum_frames=1
        )
        self._client.set_application_parameters(viewport=size)
        self._client.set_application_parameters(image_stream_fps=0)

        frame_buffer_mode, spp = self._set_renderer_params(
            renderer, samples_per_pixel, gi_length
        )
        self._client.set_renderer(max_accum_frames=spp)

        control_points = [self.get_camera()]
        current_animation_frame = int(
            self._client.get_animation_parameters()["current"]
        )
        animation_frames = [current_animation_frame]

        self.build_camera_path(
            control_points=control_points,
            nb_steps_between_control_points=1,
            smoothing_size=1,
        )

        if show_progress:
            progress_widget = IntProgress(
                description="In progress...", min=0, max=100, value=0
            )
            display(progress_widget)

        self.export_frames(
            path=path,
            base_name=base_name,
            animation_frames=animation_frames,
            size=size,
            samples_per_pixel=spp,
            export_intermediate_frames=export_intermediate_frames,
            frame_buffer_mode=frame_buffer_mode,
            keywords=keywords,
        )

        done = False
        while not done:
            time.sleep(1)
            if show_progress:
                progress = self.get_export_frames_progress()["progress"]
                progress_widget.value = progress * 100
            done = self.get_export_frames_progress()["done"]

        if show_progress:
            progress_widget.description = "Done"
            progress_widget.value = 100

        self._client.set_application_parameters(
            image_stream_fps=old_image_stream_fps, viewport=old_viewport_size
        )
        self._client.set_renderer(
            samples_per_pixel=old_samples_per_pixel,
            max_accum_frames=old_max_accum_frames,
        )
        self._smoothed_key_frames = copy.deepcopy(old_smoothed_key_frames)

    def create_movie(
        self,
        path,
        size,
        animation_frames=list(),
        quality=100,
        samples_per_pixel=1,
        start_frame=0,
        end_frame=0,
        interpupillary_distance=0.0635,
        export_intermediate_frames=True,
    ):
        """
        Create and export a set of PNG frames for later movie generation

        :path: Full path of the snapshot folder
        :size: Frame buffer size
        :animation_frames: Optional list of animation frames
        :quality: PNG quality
        :samples_per_pixel: Samples per pixel
        :start_frame: Start frame to export in the provided sequence
        :end_frame: Last frame to export in the provided sequence
        :interpupillary_distance: Interpupillary distance for stereo rendering
        :export_intermediate_frames: If True, intermediate samples are stored to disk. Otherwise,
        only the final accumulation is exported
        """
        application_params = self._client.get_application_parameters()
        renderer_params = self._client.get_renderer()

        old_image_stream_fps = application_params["image_stream_fps"]
        old_viewport_size = application_params["viewport"]
        old_samples_per_pixel = renderer_params["samples_per_pixel"]
        old_max_accum_frames = renderer_params["max_accum_frames"]
        self._client.set_renderer(
            samples_per_pixel=1, max_accum_frames=samples_per_pixel
        )
        self._client.set_application_parameters(viewport=size)
        self._client.set_application_parameters(image_stream_fps=0)

        progress_widget = IntProgress(
            description="In progress...", min=0, max=100, value=0
        )
        display(progress_widget)

        self.export_frames(
            path=path,
            base_name="",
            animation_frames=animation_frames,
            start_frame=start_frame,
            end_frame=end_frame,
            size=size,
            samples_per_pixel=samples_per_pixel,
            quality=quality,
            interpupillary_distance=interpupillary_distance,
            export_intermediate_frames=export_intermediate_frames,
        )

        done = False
        while not done:
            time.sleep(1)
            progress = self.get_export_frames_progress()["progress"]
            progress_widget.value = progress * 100
            done = self.get_export_frames_progress()["done"]

        self._client.set_application_parameters(
            image_stream_fps=old_image_stream_fps, viewport=old_viewport_size
        )
        self._client.set_renderer(
            samples_per_pixel=old_samples_per_pixel,
            max_accum_frames=old_max_accum_frames,
        )

        progress_widget.description = "Done"
        progress_widget.value = 100

    @staticmethod
    def set_image_metadata(
            file_name, description, owner, artist_name, copyright, software_name, software_version,
            keywords=list(), artist_email=None, artist_orcid=None, artist_job_description=None,
            contact_details=None, contributors=list()):
        """
        Sets the metadata for an image file.

        This method updates the metadata of an image file with the provided details. It handles
        metadata for XML, Exif, and IPTC tags.

        :param file_name: str: The name of the image file to update.
        :param description: str: The description of the image.
        :param owner: str: The owner of the image.
        :param artist_name: str: The name of the artist.
        :param copyright: str: The copyright information.
        :param software_name: str: The name of the software used.
        :param software_version: str: The version of the software used.
        :param keywords: list: A list of keywords associated with the image (default is an empty
        list).
        :param artist_email: str, optional: The email of the artist (default is None).
        :param artist_orcid: str, optional: The ORCID of the artist (default is None).
        :param artist_job_description: str, optional: The job description of the artist (default is
        None).
        :param contact_details: str, optional: The contact details (default is None).
        :param contributors: list: A list of contributors (default is an empty list).

        :return: None
        """
        metadata = pyexiv2.ImageMetadata(file_name)
        metadata.read()
        '''XML tags'''
        metadata['Xmp.dc.artist'] = artist_name
        metadata.xmp_keys.append(['Xmp.dc.ORCID', artist_orcid])
        if keywords:
            metadata['Xmp.dc.subject'] = list(keywords)

        '''Exif Tags'''
        metadata['Exif.Image.Artist'] = owner
        metadata['Exif.Image.Software'] = '%s %s' % (software_name, software_version)
        metadata['Exif.Image.Copyright'] = copyright
        metadata['Exif.Image.ImageDescription'] = description

        '''IPTC tags'''
        list_of_contributors = ''
        list_of_contributors += '%s (%s)' % (artist_name, artist_email)
        for contributor in contributors:
            list_of_contributors += ',' + contributor

        metadata['Iptc.Application2.Byline'] = [list_of_contributors]
        metadata['Iptc.Application2.Caption'] = [description]
        metadata['Iptc.Application2.Copyright'] = [copyright]
        metadata['Iptc.Application2.Program'] = [software_name]
        metadata['Iptc.Application2.ProgramVersion'] = [software_version]
        if contact_details:
            metadata['Iptc.Application2.Contact'] = [contact_details]
        if keywords:
            metadata['Iptc.Application2.Keywords'] = list(keywords)
        if artist_job_description:
            metadata['Iptc.Application2.BylineTitle'] = [artist_job_description]
        metadata.write()

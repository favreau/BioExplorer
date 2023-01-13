#!/usr/bin/env python

# -*- coding: utf-8 -*-

# The Blue Brain BioExplorer is a tool for scientists to extract and analyse
# scientific data from visualization
#
# Copyright 2020-2023 Blue BrainProject / EPFL
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
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

"""Client that connects to a remote running Brayns instance which provides the supported API."""

import rockets

from .base import BaseClient
from .utils import build_schema_requests_from_registry, convert_snapshot_response_to_PIL


class Client(BaseClient):
    """Client that connects to a remote running Brayns instance which provides the supported API."""

    def __init__(self, url, loop=None):
        """
        Create a new client instance by connecting to the given URL.

        :param str url: a string 'hostname:port' to connect to a running Brayns instance
        :param asyncio.AbstractEventLoop loop: Event loop where this client should run in
        """
        super().__init__(url)

        self.rockets_client = rockets.Client(url, subprotocols=["rockets"], loop=loop)

        registry, requests = build_schema_requests_from_registry(self.http_url)
        schemas = self.rockets_client.batch(requests)
        super()._build_api(registry, requests, schemas)

    # pylint: disable=W0613,W0622,E1101
    def image(
        self,
        size,
        format="jpg",
        animation_parameters=None,
        camera=None,
        quality=None,
        renderer=None,
        samples_per_pixel=None,
    ):
        """
        Request a snapshot from Brayns and return a PIL image.

        :param tuple size: (width,height) of the resulting image
        :param str format: image type as recognized by FreeImage
        :param object animation_parameters: animation params to use instead of current params
        :param object camera: camera to use instead of current camera
        :param int quality: compression quality between 1 (worst) and 100 (best)
        :param object renderer: renderer to use instead of current renderer
        :param int samples_per_pixel: samples per pixel to increase render quality
        :return: the PIL image of the current rendering, None on error obtaining the image
        :rtype: :py:class:`~PIL.Image.Image`
        """
        args = locals()
        del args["self"]
        result = self.snapshot(**{k: v for k, v in args.items() if v})
        return convert_snapshot_response_to_PIL(result)

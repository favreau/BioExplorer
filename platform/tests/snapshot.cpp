/*
 * Copyright (c) 2018, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Daniel.Nachbaur@epfl.ch
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

#include <jsonSerialization.h>
#include <tests/paths.h>

#include "ClientServer.h"
#include <platform/core/engineapi/Engine.h>
#include <platform/core/engineapi/Renderer.h>

#include "tests/PDiffHelpers.h"

TEST_CASE_FIXTURE(ClientServer, "snapshot")
{
    core::SnapshotParams params;
    params.format = "png";
    params.size = {50, 50};
    params.quality = 90;

    auto image =
        makeRequest<core::SnapshotParams,
                    core::ImageGenerator::ImageBase64>("snapshot", params);

    CHECK(compareBase64TestImage(image, "snapshot.png"));
}

TEST_CASE_FIXTURE(ClientServer, "snapshot_with_render_params")
{
    // move far enough away to see the background
    auto camera{std::make_unique<core::Camera>()};
    *camera = getCamera();
    camera->setPosition({0, 0, 50});

    core::SnapshotParams params;
    params.camera = std::move(camera);
    params.format = "jpg";
    params.size = {5, 5};
    params.quality = 75;
    params.name = "black_image";

    auto image =
        makeRequest<core::SnapshotParams,
                    core::ImageGenerator::ImageBase64>("snapshot", params);

    // use a red background, as opposed to the default black
    auto renderingParams{std::make_unique<core::RenderingParameters>()};
    renderingParams->setBackgroundColor({1, 0, 0});
    params.renderingParams = std::move(renderingParams);
    params.name = "red_image";
    auto image_with_red_background =
        makeRequest<core::SnapshotParams,
                    core::ImageGenerator::ImageBase64>("snapshot", params);

    CHECK_NE(image.data, image_with_red_background.data);
}

TEST_CASE_FIXTURE(ClientServer, "snapshot_empty_params")
{
    CHECK_THROWS_AS((makeRequest<core::SnapshotParams,
                                 core::ImageGenerator::ImageBase64>(
                        "snapshot", core::SnapshotParams())),
                    rockets::jsonrpc::response_error);
}

TEST_CASE_FIXTURE(ClientServer, "snapshot_illegal_format")
{
    core::SnapshotParams params;
    params.size = {5, 5};
    params.format = "";
    CHECK_THROWS_AS(
        (makeRequest<core::SnapshotParams,
                     core::ImageGenerator::ImageBase64>("snapshot", params)),
        rockets::jsonrpc::response_error);
}
/*
 * Copyright (c) 2015-2023, EPFL/Blue Brain Project
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

#include "AddModelTask.h"

#include "Errors.h"
#include "LoadModelFunctor.h"

#include <platform/core/engineapi/Engine.h>
#include <platform/core/engineapi/Model.h>
#include <platform/core/engineapi/Scene.h>

namespace core
{
AddModelTask::AddModelTask(const ModelParams& modelParams, Engine& engine)
{
    const auto& registry = engine.getScene().getLoaderRegistry();

    // pre-check for validity of given paths
    const auto& path = modelParams.getPath();
    if (path.empty())
        throw MISSING_PARAMS;

    if (!registry.isSupportedFile(path))
        throw UNSUPPORTED_TYPE;

    LoadModelFunctor functor{engine, modelParams};
    functor.setCancelToken(_cancelToken);
    functor.setProgressFunc([&progress = progress](const auto& msg, auto, auto amount)
                            { progress.update(msg, amount); });

    // load data, return model descriptor
    _task = async::spawn(std::move(functor))
                .then(
                    [&engine](async::task<ModelDescriptorPtr> result)
                    {
                        engine.triggerRender();
                        return result.get();
                    });
}
} // namespace core
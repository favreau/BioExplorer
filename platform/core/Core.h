/*
 *
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * This file is part of Blue Brain BioExplorer <https://github.com/BlueBrain/BioExplorer>
 *
 * Copyright 2020-2023 Blue BrainProject / EPFL
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

#include <platform/core/common/Api.h>
#include <platform/core/common/Types.h>

namespace core
{
/**
    Core is a minimalistic library that allows optimized ray-tracing rendering
    of meshes and parametric geometry. Core provides an abstraction of the
    underlying rendering engines, making it possible to use the best rendering
    engine depending on the case.

    Core uses plugins for extended function. There are a few built-in plugins
    and additional plugins can be dynamically loaded.

    The underlying rendering engine (OSPRay, Optix, FireRays, etc) is specified
    in the rendering parameters and is invoked by the render method for
    generating the frames.

    Underlying rendering engines support CPU, GPU and heterogeneous
    architectures

    This object exposes the basic API for Core
*/
class Core
{
public:
    /** Core instance initialization.
     *
     * Initialization involves command line parsing, engine creation, plugin
     * loading and initialization, data loading, scene creation and setup of
     * keyboard mouse interactions.
     *
     * In a setup using event loops, the event loop must be set up correctly
     * before calling this constructor to ensure that plugins can install their
     * event callbacks successfully.
     *
     * Command line parameters provide options about the application itself,
     * the geometry and the renderer. Core creates the scene using built-in
     * and plug-in provided loaders.
     */
    PLATFORM_API Core(int argc, const char** argv);
    PLATFORM_API ~Core();

    /** @name Simple execution API  */
    //@{
    /**
     * Renders color and depth buffers of the current scene, according to
     * specified parameters.
     *
     * Combines commit() and render() together in a synchronized fashion.
     *
     * @param renderInput Rendering parameters such as the position of the
     *        camera and according model and projection matrices
     * @param renderOutput Color and depth buffers
     */
    PLATFORM_API void commitAndRender(const RenderInput& renderInput, RenderOutput& renderOutput);

    /**
     * Renders color and depth buffers of the current scene, according to
     * default parameters. This is typically used by an application that does
     * not provide any on-screen visualization. In such cases, input and output
     * parameters are provided by network events. For instance, a camera event
     * defines the origin, target and up vector of the camera, and an ImageJPEG
     * event triggers the rendering and gathers the results in a form of a
     * base64 encoded JPEG image.
     *
     * Combines commit(), render() and postRender() together in a synchronized
     * fashion.
     *
     * @return true if rendering should continue or false if user inputs
     *         requested to stop.
     */
    PLATFORM_API bool commitAndRender();
    //@}

    /** @name Low-level execution API */
    //@{
    /**
     * Handle events, update animation, call preRender() on plugins and commit
     * changes on the engine, scene, camera, renderer, etc. to prepare rendering
     * of a new frame.
     *
     * @return true if render() is allowed/needed after all states have been
     *         evaluated (accum rendering, data loading, etc.)
     * @note threadsafe with render()
     */
    PLATFORM_API bool commit();

    /**
     * Render a frame into the current framebuffer.
     * @note threadsafe with commit()
     */
    PLATFORM_API void render();

    /**
     * Call postRender() on engine and plugins to signal finish of render().
     * Shall only be called after render() has finished. This is only needed if
     * commit() and render() are called individually.
     */
    PLATFORM_API void postRender();
    //@}

    /**
       @return the current engine
    */
    PLATFORM_API Engine& getEngine();

    /**
     * @return The parameter manager
     */
    PLATFORM_API ParametersManager& getParametersManager();

    /**
     * Gets the keyboard handler
     */
    PLATFORM_API KeyboardHandler& getKeyboardHandler();

    /**
     * Gets the camera manipulator
     */
    PLATFORM_API AbstractManipulator& getCameraManipulator();

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};
} // namespace core

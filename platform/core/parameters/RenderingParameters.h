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

#include "AbstractParameters.h"
#include <deque>

SERIALIZATION_ACCESS(RenderingParameters)

namespace core
{
class AbstractParameters;

/** Manages rendering parameters
 */
class RenderingParameters : public AbstractParameters
{
public:
    RenderingParameters();

    /** @copydoc AbstractParameters::print */
    void print() final;

    const std::string& getCurrentRenderer() const { return _renderer; }
    void setCurrentRenderer(const std::string& renderer) { _updateValue(_renderer, renderer); }

    /** All registered renderers */
    const auto& getRenderers() const { return _renderers; }
    void addRenderer(const std::string& renderer)
    {
        if (std::find(_renderers.begin(), _renderers.end(), renderer) == _renderers.end())
            _renderers.push_front(renderer);
    }
    const std::string& getCurrentCamera() const { return _camera; }

    /** All registered cameras */
    const auto& getCameras() const { return _cameras; }
    void addCamera(const std::string& camera) { _cameras.push_front(camera); }

    /** Number of samples per pixel */
    uint32_t getSamplesPerPixel() const { return _spp; }
    void setSamplesPerPixel(const uint32_t value) { _updateValue(_spp, std::max(1u, value)); }

    /** Sub-sampling */
    uint32_t getSubsampling() const { return _subsampling; }
    void setSubsampling(const uint32_t subsampling) { _updateValue(_subsampling, std::max(1u, subsampling)); }

    /** Background color */
    const Vector3d& getBackgroundColor() const { return _backgroundColor; }
    void setBackgroundColor(const Vector3d& value) { _updateValue(_backgroundColor, value); }

    /**
       Light source follow camera origin
    */
    bool getHeadLight() const { return _headLight; }
    void setHeadLight(const bool value) { _updateValue(_headLight, value); }

    /** If the rendering should be refined by accumulating multiple passes */
    bool getAccumulation() const { return _accumulation; }
    /**
     * @return the threshold where accumulation stops if the variance error
     * reaches this value.
     */
    double getVarianceThreshold() const { return _varianceThreshold; }
    /**
     * The threshold where accumulation stops if the variance error reaches this
     * value.
     */
    void setVarianceThreshold(const double value) { _updateValue(_varianceThreshold, value); }

    /**
     * The maximum number of accumulation frames before engine signals to stop
     * continuation of rendering.
     *
     * @sa Engine::continueRendering()
     */
    void setMaxAccumFrames(const size_t value) { _updateValue(_maxAccumFrames, value); }
    size_t getMaxAccumFrames() const { return _maxAccumFrames; }

    /** If the rendering should be refined by accumulating multiple passes */
    AccumulationType getAccumulationType() const { return _accumulationType; }
    const std::string getAccumulationTypeAsString(const AccumulationType value);

    /**
     *  Denoising parameters: Used by the Optix 6 engine only
     */
    /** Number of frames that show the original image before switching on denoising */
    void setNumNonDenoisedFrames(const uint32_t value) { _updateValue(_numNonDenoisedFrames, value); }
    uint32_t getNumNonDenoisedFrames() const { return _numNonDenoisedFrames; }

    /* Amount of the original image that is blended with the denoised result ranging from 0.0 to 1.0 */
    void setDenoiseBlend(const float value) { _updateValue(_denoiseBlend, value); }
    float getDenoiseBlend() const { return _denoiseBlend; }

    /* Tone mapper exposure */
    void setToneMapperExposure(const float value) { _updateValue(_toneMapperExposure, value); }
    float getToneMapperExposure() const { return _toneMapperExposure; }

    /* Tone mapper gamma */
    void setToneMapperGamma(const float value) { _updateValue(_toneMapperGamma, value); }
    float getToneMapperGamma() const { return _toneMapperGamma; }

protected:
    void parse(const po::variables_map& vm) final;

    std::string _renderer{"basic"};
    std::deque<std::string> _renderers;
    std::string _camera{"perspective"};
    std::deque<std::string> _cameras;
    uint32_t _spp{1};
    uint32_t _subsampling{1};
    bool _accumulation{true};
    Vector3d _backgroundColor{0., 0., 0.};
    bool _headLight{true};
    double _varianceThreshold{-1.};
    size_t _maxAccumFrames{100};
    uint32_t _numNonDenoisedFrames{2};
    float _denoiseBlend{0.1f};
    float _toneMapperExposure{1.5f};
    float _toneMapperGamma{1.f};
    AccumulationType _accumulationType{AccumulationType::linear};

    SERIALIZATION_FRIEND(RenderingParameters)
};
} // namespace core

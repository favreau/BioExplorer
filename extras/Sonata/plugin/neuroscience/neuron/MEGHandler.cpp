/*
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
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

#include "MEGHandler.h"

#include <common/Logs.h>
#include <common/Utils.h>

#include <plugin/neuroscience/common/Types.h>

#include <bioexplorer/core/plugin/common/Octree.h>

#include <brayns/common/scene/ClipPlane.h>
#include <brayns/engineapi/Model.h>

#include <brain/brain.h>
#include <brion/brion.h>

#include <fstream>

namespace sonataexplorer
{
namespace neuroscience
{
namespace neuron
{
using namespace brion;
using namespace brain;
using namespace common;

const float DEFAULT_EVENT_VALUE = 1.f;
const float DEFAULT_VOLTAGE_REST_VALUE = -80.f;

std::string Vector3fToString(const Vector3f& v)
{
    return "[" + std::to_string(v.x) + "," + std::to_string(v.y) + "," +
           std::to_string(v.z) + "]";
}

MEGHandler::MEGHandler(const std::string& circuitConfiguration,
                       const std::string& reportName, const bool synchronous)
    : AbstractSimulationHandler()
    , _synchronousMode(synchronous)
    , _ready(false)
{
    const brion::BlueConfig blueConfiguration(circuitConfiguration);
    const brain::Circuit circuit(blueConfiguration);

    // Report
    const auto& voltageReport = blueConfiguration.getReportSource(reportName);
    PLUGIN_INFO("Voltage report: " << voltageReport);
    _report =
        std::make_shared<brion::CompartmentReport>(brion::URI(voltageReport),
                                                   brion::MODE_READ,
                                                   circuit.getGIDs());

    const brain::GIDSet& simulatedGids = _report->getGIDs();
    _transformations = circuit.getTransforms(simulatedGids);

    _dt = _report->getTimestep();
    const auto startTime = _report->getStartTime();
    const auto endTime = _report->getEndTime();
    _startFrame = startTime / _dt;
    _nbFrames = (endTime - startTime) / _dt;
    _unit = _report->getTimeUnit();
    _frameSize = _report->getFrameSize();

    PLUGIN_INFO("-----------------------------------------------------------");
    PLUGIN_INFO("Voltage simulation information");
    PLUGIN_INFO("------------------------------");
    PLUGIN_INFO("Report name          : " << reportName);
    PLUGIN_INFO("Start time           : " << startTime);
    PLUGIN_INFO("End time             : " << endTime);
    PLUGIN_INFO("Delta between frames : " << _dt);
    PLUGIN_INFO("Number of frames     : " << _nbFrames);
    PLUGIN_INFO("Start frame          : " << _startFrame);
    PLUGIN_INFO("Frame size           : " << _frameSize);
    PLUGIN_INFO("Synchronous loading  : " << (_synchronousMode ? "ON" : "OFF"));
    PLUGIN_INFO("-----------------------------------------------------------");
}

MEGHandler::MEGHandler(const MEGHandler& rhs)
    : AbstractSimulationHandler(rhs)
{
}

MEGHandler::~MEGHandler() {}

ModelMetadata MEGHandler::buildModel(Model& model, const double voxelSize,
                                     const double density)
{
    _voxelSize = voxelSize;
    _density = density;

    if (density > 1.f || density <= 0.f)
        PLUGIN_THROW("Density should be higher > 0 and <= 1");

    _bounds.reset();
    for (const auto& transformation : _transformations)
    {
        const Vector3f position = get_translation(transformation);
        _bounds.merge(position + DEFAULT_EVENT_VALUE);
        _bounds.merge(position - DEFAULT_EVENT_VALUE);
    }

    // Extend bounds to 200%
    _bounds.merge(_bounds.getCenter() + _bounds.getSize());
    _bounds.merge(_bounds.getCenter() - _bounds.getSize());

    const size_t materialId = 0;
    auto material = model.createMaterial(materialId, "MEG");
    TriangleMesh mesh = createBox(_bounds.getMin(), _bounds.getMax());
    model.getTriangleMeshes()[materialId] = mesh;
    model.updateBounds();

    ModelMetadata metadata = {
        {"Scene AABB", Vector3fToString(_bounds.getMin()) + ", " +
                           Vector3fToString(_bounds.getMax())},
        {"Scene dimension", Vector3fToString(_bounds.getSize())},
        {"Element spacing ", Vector3fToString(_spacing)},
        {"Volume dimensions", Vector3fToString(_dimensions)},
        {"Element offset", Vector3fToString(_offset)},
        {"Data size", std::to_string(_frameSize)}};
    return metadata;
}

void MEGHandler::_buildOctree()
{
    const auto voltages = std::move(*_currentFrameFuture.get().data);
    if (voltages.size() != _transformations.size())
        PLUGIN_ERROR("Invalid number of values: " << voltages.size()
                                                  << " instead of "
                                                  << _transformations.size());
    uint64_t index = 0;
    const uint32_t densityRatio = 1.f / _density;
    floats events;
    for (const auto& transformation : _transformations)
    {
        if (index % densityRatio == 0)
        {
            ++index;
            continue;
        }
        const Vector3f position = get_translation(transformation);
        const auto value = voltages[index] - DEFAULT_VOLTAGE_REST_VALUE;
        events.push_back(position.x);
        events.push_back(position.y);
        events.push_back(position.z);
        events.push_back(value);
        events.push_back(value);
        ++index;
    }

    const ::bioexplorer::common::Octree accelerator(events, _voxelSize,
                                                    _bounds.getMin(),
                                                    _bounds.getMax());
    const uint32_t volumeSize = accelerator.getVolumeSize();
    _offset = _bounds.getMin();
    _dimensions = accelerator.getVolumeDimensions();
    _spacing = Vector3f(_bounds.getSize()) / Vector3f(_dimensions);

    const auto& indices = accelerator.getFlatIndices();
    const auto& data = accelerator.getFlatData();
    _frameData.clear();
    _frameData.push_back(_offset.x);
    _frameData.push_back(_offset.y);
    _frameData.push_back(_offset.z);
    _frameData.push_back(_spacing.x);
    _frameData.push_back(_spacing.y);
    _frameData.push_back(_spacing.z);
    _frameData.push_back(_dimensions.x);
    _frameData.push_back(_dimensions.y);
    _frameData.push_back(_dimensions.z);
    _frameData.push_back(accelerator.getOctreeSize());
    _frameData.push_back(indices.size());
    _frameData.insert(_frameData.end(), indices.begin(), indices.end());
    _startDataIndex = _frameData.size();
    _frameData.insert(_frameData.end(), data.begin(), data.end());
    _frameSize = _frameData.size();
}

void* MEGHandler::getFrameData(const uint32_t frame)
{
    const auto boundedFrame = _startFrame + _getBoundedFrame(frame);

    if (!_currentFrameFuture.valid() && _currentFrame != boundedFrame)
        _triggerLoading(boundedFrame);

    if (!_makeFrameReady(boundedFrame))
        return nullptr;

    return _frameData.data();
}

void MEGHandler::_triggerLoading(const uint32_t frame)
{
    float timestamp = frame * _dt;
    timestamp = std::min(static_cast<float>(_nbFrames), timestamp);

    if (_currentFrameFuture.valid())
        _currentFrameFuture.wait();

    _ready = false;
    _currentFrameFuture = _report->loadFrame(timestamp);
}

bool MEGHandler::_isFrameLoaded() const
{
    if (!_currentFrameFuture.valid())
        return false;

    if (_synchronousMode)
    {
        _currentFrameFuture.wait();
        return true;
    }

    return _currentFrameFuture.wait_for(std::chrono::milliseconds(0)) ==
           std::future_status::ready;
}

bool MEGHandler::_makeFrameReady(const uint32_t frame)
{
    if (_isFrameLoaded())
    {
        try
        {
            _buildOctree();
        }
        catch (const std::exception& e)
        {
            PLUGIN_ERROR("Error loading simulation frame " << frame << ": "
                                                           << e.what());
            return false;
        }
        _currentFrame = frame;
        _ready = true;
    }
    return true;
}

AbstractSimulationHandlerPtr MEGHandler::clone() const
{
    return std::make_shared<MEGHandler>(*this);
}
} // namespace neuron
} // namespace neuroscience
} // namespace sonataexplorer
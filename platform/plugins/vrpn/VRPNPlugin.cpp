/*
    Copyright 2018 - 2024 Blue Brain Project / EPFL

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "VRPNPlugin.h"

#include <platform/core/common/Logs.h>
#include <platform/core/engineapi/Camera.h>
#include <platform/core/pluginapi/Plugin.h>

namespace core
{
namespace
{
constexpr vrpn_int32 HEAD_SENSOR_ID = 0;
constexpr vrpn_int32 FLYSTICK_SENSOR_ID = 1;
constexpr double MOVING_SPEED = 1.0f;
const std::string DEFAULT_VRPN_NAME = "DTrack@cave1";
#ifdef BRAYNSVRPN_USE_LIBUV
constexpr int VRPN_IDLE_TIMEOUT_MS = 5000;
constexpr int VRPN_REPEAT_TIMEOUT_MS = 16;
#endif

const std::string HEAD_POSITION_PROP = "headPosition";
const std::string HEAD_ROTATION_PROP = "headRotation";

constexpr std::array<double, 3> to_array_3d(const vrpn_float64* pos)
{
    return {{pos[0], pos[1], pos[2]}};
}
constexpr std::array<double, 4> to_array_4d(const vrpn_float64* quat)
{
    return {{quat[0], quat[1], quat[2], quat[3]}};
}

void trackerCallback(void* userData, const vrpn_TRACKERCB tracker)
{
    auto camera = static_cast<Camera*>(userData);
    camera->updateProperty(HEAD_POSITION_PROP, to_array_3d(tracker.pos));
    camera->updateProperty(HEAD_ROTATION_PROP, to_array_4d(tracker.quat));
}

void flyStickCallback(void* userData, const vrpn_TRACKERCB tracker)
{
    VrpnStates* states = static_cast<VrpnStates*>(userData);
    states->flyStickOrientation = glm::quat(tracker.quat[3], tracker.quat[0], tracker.quat[1], tracker.quat[2]);
}

void joystickCallback(void* userData, const vrpn_ANALOGCB joystick)
{
    VrpnStates* states = static_cast<VrpnStates*>(userData);
    states->axisX = joystick.channel[0];
    states->axisZ = joystick.channel[1];
}
} // namespace

VRPNPlugin::VRPNPlugin(const std::string& vrpnName)
    : _vrpnName(vrpnName)
{
}

VRPNPlugin::~VRPNPlugin()
{
    _vrpnTracker->unregister_change_handler(&(_api->getCamera()), trackerCallback, HEAD_SENSOR_ID);
}

void VRPNPlugin::init()
{
    _vrpnTracker = std::make_unique<vrpn_Tracker_Remote>(_vrpnName.c_str());
    if (!_vrpnTracker->connectionPtr()->doing_okay())
        throw std::runtime_error("VRPN couldn't connect to: " + _vrpnName + " tracker");

    _vrpnAnalog = std::make_unique<vrpn_Analog_Remote>(_vrpnName.c_str());
    if (!_vrpnAnalog->connectionPtr()->doing_okay())
        throw std::runtime_error("VRPN couldn't connect to: " + _vrpnName + " analog");

    CORE_INFO("VRPN successfully connected to " << _vrpnName);

#ifdef BRAYNSVRPN_USE_LIBUV
    _setupIdleTimer();
#endif

    _vrpnTracker->register_change_handler(&(_api->getCamera()), trackerCallback, HEAD_SENSOR_ID);
    _vrpnTracker->register_change_handler(&_states, flyStickCallback, FLYSTICK_SENSOR_ID);
    _vrpnAnalog->register_change_handler(&_states, joystickCallback);
}

void VRPNPlugin::preRender()
{
    _timer.stop();
    _vrpnTracker->mainloop();

    double frameTime = _timer.seconds();

    Camera& camera = _api->getCamera();
    Vector3d pos = camera.getPosition();
    pos += _states.axisX * MOVING_SPEED * glm::rotate(_states.flyStickOrientation, Vector3f(1.0, 0.0, 0.0)) * frameTime;
    pos +=
        _states.axisZ * MOVING_SPEED * glm::rotate(_states.flyStickOrientation, Vector3f(0.0, 0.0, -1.0)) * frameTime;
    camera.setPosition(pos);

    _timer.start();
}

#ifdef BRAYNSVRPN_USE_LIBUV
void VRPNPlugin::resumeRenderingIfTrackerIsActive()
{
    _vrpnTracker->mainloop();
    if (_api->getCamera().isModified())
        _api->triggerRender();
}

void VRPNPlugin::_setupIdleTimer()
{
    if (auto uvLoop = uv_default_loop())
    {
        _idleTimer.reset(new uv_timer_t);
        uv_timer_init(uvLoop, _idleTimer.get());
        _idleTimer->data = this;

        uv_timer_start(
            _idleTimer.get(),
            [](uv_timer_t* timer)
            {
                auto plugin = static_cast<VRPNPlugin*>(timer->data);
                plugin->resumeRenderingIfTrackerIsActive();
            },
            VRPN_IDLE_TIMEOUT_MS, VRPN_REPEAT_TIMEOUT_MS);
    }
}
#endif
} // namespace core

extern "C" core::ExtensionPlugin* brayns_plugin_create(const int argc, const char** argv)
{
    if (argc > 2)
    {
        throw std::runtime_error(
            "VRPN plugin expects at most one argument, the name of the VRPN "
            "device to connect to (eg: Tracker0@localhost)");
    }

    const auto vrpnName = (argc >= 2) ? argv[1] : core::DEFAULT_VRPN_NAME;
    return new core::VRPNPlugin(vrpnName);
}

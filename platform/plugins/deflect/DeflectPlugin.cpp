/*
    Copyright 2015 - 2024 Blue Brain Project / EPFL

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

#include "DeflectPlugin.h"
#include "DeflectParameters.h"
#include "utils.h"

#include <platform/core/common/ActionInterface.h>
#include <platform/core/common/PropertyMap.h>
#include <platform/core/common/input/KeyboardHandler.h>
#include <platform/core/common/utils/Utils.h>
#include <platform/core/engineapi/Engine.h>
#include <platform/core/engineapi/FrameBuffer.h>
#include <platform/core/manipulators/AbstractManipulator.h>

#include <platform/core/parameters/ParametersManager.h>

#include <platform/core/pluginapi/Plugin.h>

#ifdef USE_NETWORKING
#include <uvw.hpp>
#endif

namespace
{
const float wheelFactor = 1.f / 40.f;
}

namespace core
{
class DeflectPlugin::Impl
{
public:
    Impl(PluginAPI* api, DeflectParameters&& params)
        : _api(*api)
        , _engine(api->getEngine())
        , _appParams{api->getParametersManager().getApplicationParameters()}
        , _params(std::move(params))
    {
        if (auto ai = api->getActionInterface())
        {
            const RpcParameterDescription desc{"stream", "Stream to a displaywall", Execution::sync, "param",
                                               "Stream parameters"};
            ai->registerNotification(desc, _params.getPropertyMap(),
                                     [&](const PropertyMap& prop)
                                     {
                                         _params.getPropertyMap().merge(prop);
                                         _params.markModified();
                                         _engine.triggerRender();
                                     });
        }
    }

    void preRender()
    {
        if (_stream && utils::needsReset(*_stream, _params))
            _closeStream();

        if (_params.getEnabled() && !_stream)
            _startStream();

        if (_stream)
            _handleDeflectEvents();

        if (_params.usePixelOp() && _params.isModified())
        {
            for (auto frameBuffer : _engine.getFrameBuffers())
                frameBuffer->updatePixelOp(_params.getPropertyMap());
        }

        _params.resetModified();
    }

    void postRender()
    {
        if (!_params.usePixelOp() && _stream)
            _sendDeflectFrame(_engine);
    }

private:
    struct Image
    {
        std::vector<char> data;
        Vector2ui size;
        FrameBufferFormat format;
    };

    void _startStream()
    {
        try
        {
            if (_params.usePixelOp())
            {
                _stream.reset(new deflect::Observer(_params.getId(), _params.getHostname(), _params.getPort()));
            }
            else
            {
                _stream.reset(new deflect::Stream(_params.getId(), _params.getHostname(), _params.getPort()));
            }

            if (_stream->registerForEvents())
                _setupSocketListener();
            else
                CORE_ERROR("Deflect failed to register for events!");

            _params.setId(_stream->getId());
            _params.setHost(_stream->getHost());

            // distributed streaming requires a properly setup stream ID (either
            // from DEFLECT_ID env variable or from here)
            if (_params.usePixelOp() && !_params.getId().empty())
            {
                for (auto frameBuffer : _engine.getFrameBuffers())
                {
                    // Use format 'none' for the per-tile streaming to avoid
                    // tile readback to the MPI master
                    frameBuffer->setFormat(FrameBufferFormat::none);
                    frameBuffer->createPixelOp(TEXTIFY(deflectPixelOp));
                    frameBuffer->updatePixelOp(_params.getPropertyMap());
                }
            }

            _sendSizeHints();

            CORE_INFO("Deflect successfully connected to Tide on host " << _stream->getHost());
        }
        catch (const std::runtime_error& ex)
        {
            CORE_ERROR("Deflect failed to initialize. " << ex.what());
            _params.setEnabled(false);
        }
    }

    void _closeStream()
    {
        CORE_INFO("Closing Deflect stream");

        _waitOnFutures();
        _lastImages.clear();
#ifdef USE_NETWORKING
        if (_pollHandle)
        {
            _pollHandle->stop();
            _pollHandle.reset();
        }
#endif
        _stream.reset();
    }

    void _setupSocketListener()
    {
#ifdef USE_NETWORKING
        assert(_stream->isConnected());

        auto loop = uvw::Loop::getDefault();
        _pollHandle = loop->resource<uvw::PollHandle>(_stream->getDescriptor());

        _pollHandle->on<uvw::PollEvent>([&engine = _engine](const auto&, auto&) { engine.triggerRender(); });

        _pollHandle->start(uvw::PollHandle::Event::READABLE);

        _stream->setDisconnectedCallback(
            [&]
            {
                _pollHandle->stop();
                _pollHandle.reset();
            });
#endif
    }

    void _handleDeflectEvents()
    {
        auto& cameraManipulator = _api.getCameraManipulator();
        const auto& windowSize = _appParams.getWindowSize();
        while (_stream->hasEvent())
        {
            const deflect::Event& event = _stream->getEvent();
            switch (event.type)
            {
            case deflect::Event::EVT_PRESS:
                _previousPos = _getWindowPos(event, windowSize);
                _pan = _pinch = false;
                break;
            case deflect::Event::EVT_MOVE:
            case deflect::Event::EVT_RELEASE:
            {
                const auto pos = _getWindowPos(event, windowSize);
                if (!_pan && !_pinch)
                    cameraManipulator.dragLeft(pos, _previousPos);
                _previousPos = pos;
                _pan = _pinch = false;
                break;
            }
            case deflect::Event::EVT_PAN:
            {
                if (_pinch)
                    break;
                const auto pos = _getWindowPos(event, windowSize);
                cameraManipulator.dragMiddle(pos, _previousPos);
                _previousPos = pos;
                _pan = true;
                break;
            }
            case deflect::Event::EVT_PINCH:
            {
                if (_pan)
                    break;
                const auto pos = _getWindowPos(event, windowSize);
                const auto delta = _getZoomDelta(event, windowSize);
                cameraManipulator.wheel(pos, delta * wheelFactor);
                _pinch = true;
                break;
            }
            case deflect::Event::EVT_KEY_PRESS:
            {
                _api.getKeyboardHandler().handleKeyboardShortcut(event.text[0]);
                break;
            }
            case deflect::Event::EVT_VIEW_SIZE_CHANGED:
            {
                Vector2ui newSize(event.dx, event.dy);
                if (_params.isResizingEnabled())
                    _appParams.setWindowSize(newSize);
                break;
            }
            case deflect::Event::EVT_CLOSE:
            {
                _params.setEnabled(false);
                _closeStream();
                return;
            }
            default:
                break;
            }
        }
    }

    void _sendSizeHints()
    {
        const auto& frameBuffers = _engine.getFrameBuffers();
        if (frameBuffers.empty())
            return;

        const auto& minSize = _engine.getMinimumFrameSize();

        auto sizeHints = deflect::SizeHints();
        sizeHints.maxWidth = std::numeric_limits<unsigned int>::max();
        sizeHints.maxHeight = std::numeric_limits<unsigned int>::max();
        sizeHints.minWidth = minSize.x;
        sizeHints.minHeight = minSize.y;

        // only send preferred size if we have no multi-channel setup (e.g.
        // OpenDeck)
        const uint8_t channel = utils::getChannel(frameBuffers[0]->getName());
        Vector2ui preferredSize = frameBuffers[0]->getSize();
        for (auto frameBuffer : frameBuffers)
        {
            if (channel != utils::getChannel(frameBuffer->getName()))
            {
                preferredSize = {0, 0};
                break;
            }
        }

        sizeHints.preferredWidth = preferredSize.x;
        sizeHints.preferredHeight = preferredSize.y;
        _stream->sendSizeHints(sizeHints);
    }

    void _sendDeflectFrame(Engine& engine)
    {
        const bool error = _waitOnFutures();

        if (error)
        {
            if (!_stream->isConnected())
            {
                CORE_INFO("Stream closed, exiting.");
            }
            else
            {
                CORE_ERROR("failure in _sendDeflectFrame()");
                _params.setEnabled(false);
            }
            return;
        }

        const auto& frameBuffers = engine.getFrameBuffers();
        for (size_t i = 0; i < frameBuffers.size(); ++i)
        {
            auto frameBuffer = frameBuffers[i];
            frameBuffer->map();
            if (frameBuffer->getColorBuffer())
            {
                const deflect::View view = utils::getView(frameBuffer->getName());
                const uint8_t channel = utils::getChannel(frameBuffer->getName());

                if (i <= _lastImages.size())
                    _lastImages.push_back({});
                auto& image = _lastImages[i];
                _copyToImage(image, *frameBuffer);
                _futures.push_back(_sendImage(image, view, channel));
            }
            frameBuffer->unmap();
        }
        _futures.push_back(static_cast<deflect::Stream&>(*_stream).finishFrame());
    }

    void _copyToImage(Image& image, FrameBuffer& frameBuffer)
    {
        const auto& size = frameBuffer.getSize();
        const size_t bufferSize = size.x * size.y * frameBuffer.getColorDepth();
        const auto data = frameBuffer.getColorBuffer();

        image.data.resize(bufferSize);
        memcpy(image.data.data(), data, bufferSize);
        image.size = size;
        image.format = frameBuffer.getFrameBufferFormat();
    }

    deflect::Stream::Future _sendImage(const Image& image, const deflect::View& view, const uint8_t channel)
    {
        const auto format = _getDeflectImageFormat(image.format);

        deflect::ImageWrapper deflectImage(image.data.data(), image.size.x, image.size.y, format);

        deflectImage.view = view;
        deflectImage.channel = channel;
        deflectImage.compressionQuality = _params.getQuality();
        deflectImage.compressionPolicy = _params.getCompression() ? deflect::COMPRESSION_ON : deflect::COMPRESSION_OFF;
        deflectImage.rowOrder = _params.isTopDown() ? deflect::RowOrder::top_down : deflect::RowOrder::bottom_up;
        deflectImage.subsampling = _params.getChromaSubsampling();

        return static_cast<deflect::Stream&>(*_stream).send(deflectImage);
    }

    deflect::PixelFormat _getDeflectImageFormat(const FrameBufferFormat format) const
    {
        switch (format)
        {
        case FrameBufferFormat::bgra_i8:
            return deflect::BGRA;
        case FrameBufferFormat::rgb_i8:
            return deflect::RGB;
        default:
            return deflect::RGBA;
        }
    }

    Vector2d _getWindowPos(const deflect::Event& event, const Vector2ui& windowSize) const
    {
        return {event.mouseX * windowSize.x, event.mouseY * windowSize.y};
    }

    double _getZoomDelta(const deflect::Event& pinchEvent, const Vector2ui& windowSize) const
    {
        const auto dx = pinchEvent.dx * windowSize.x;
        const auto dy = pinchEvent.dy * windowSize.y;
        return std::copysign(std::sqrt(dx * dx + dy * dy), dx + dy);
    }

    bool _waitOnFutures()
    {
        bool error = false;
        for (auto& future : _futures)
        {
            if (!future.get())
                error = true;
        }
        _futures.clear();
        return error;
    }

    PluginAPI& _api;
    Engine& _engine;
    ApplicationParameters& _appParams;
    DeflectParameters _params;
    Vector2d _previousPos;
    bool _pan = false;
    bool _pinch = false;
    std::unique_ptr<deflect::Observer> _stream;
    std::vector<Image> _lastImages;
    std::vector<deflect::Stream::Future> _futures;

#ifdef USE_NETWORKING
    std::shared_ptr<uvw::PollHandle> _pollHandle;
#endif
};

DeflectPlugin::DeflectPlugin(DeflectParameters&& params)
    : _params(std::move(params))
{
}

void DeflectPlugin::init()
{
    _impl = std::make_shared<Impl>(_api, std::move(_params));
}

void DeflectPlugin::preRender()
{
    _impl->preRender();
}

void DeflectPlugin::postRender()
{
    _impl->postRender();
}
} // namespace core

extern "C" core::ExtensionPlugin* brayns_plugin_create(const int argc, const char** argv)
{
    core::DeflectParameters params;
    if (!params.getPropertyMap().parse(argc, argv))
        return nullptr;
    return new core::DeflectPlugin(std::move(params));
}

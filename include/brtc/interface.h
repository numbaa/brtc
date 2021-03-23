#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <memory>

#include <bco/net/proactor/select.h>
#include <bco/context.h>

#include <brtc/frame.h>

namespace brtc
{

class VideoCaptureInterface {
public:
    virtual Frame capture_one_frame() = 0;
    virtual void release_frame() = 0;
};

class VideoDecoderInterface {
public:
    virtual Frame decode_one_frame(Frame frame) = 0;
};

class VideoEncoderInterface {
public:
    virtual Frame encode_one_frame(Frame frame) = 0;
};


class RenderInterface {
public:
    virtual void render_one_frame(Frame frame) = 0;
};

class MediaReceiverImpl;

class MediaReceiver {
public:
    MediaReceiver(
        std::unique_ptr<VideoDecoderInterface>&& decoder,
        std::unique_ptr<RenderInterface>&& render,
        std::shared_ptr<bco::Context<bco::net::Select>> network_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> decode_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> render_ctx);
    void start();
    void stop();

private:
    std::shared_ptr<MediaReceiverImpl> impl_;
};

class MediaSenderImpl;

class MediaSender  {
public:
    MediaSender(
        std::unique_ptr<VideoEncoderInterface>&& encoder,
        std::unique_ptr<VideoCaptureInterface>&& capture,
        std::shared_ptr<bco::Context<bco::net::Select>> network_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> encode_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> pacer_ctx);
    void start();
    void stop();

private:
    std::shared_ptr<MediaSenderImpl> impl_;
};


} // namespace brtc

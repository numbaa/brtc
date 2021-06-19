#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <memory>

//#include <bco/net/proactor/select.h>
#include <bco/net/udp.h>
#include <bco/context.h>

#include <brtc/frame.h>
#include <brtc/video/codec_interface.h>

namespace brtc
{

struct TransportInfo {
    bco::net::UdpSocket<bco::net::Select> socket;
    bco::net::Address remote_addr;
};

class VideoCaptureInterface {
public:
    virtual ~VideoCaptureInterface() { }
    virtual Frame capture_one_frame() = 0;
    virtual void release_frame() = 0;
};

class VideoDecoderInterface {
public:
    virtual ~VideoDecoderInterface() { }
    virtual Frame decode_one_frame(Frame frame) = 0;
};

class VideoEncoderInterface {
public:
    virtual ~VideoEncoderInterface() { }
    virtual EncodedFrame encode_one_frame(Frame frame) = 0;
};


class RenderInterface {
public:
    virtual ~RenderInterface() { }
    virtual void render_one_frame(Frame frame) = 0;
};

class Strategies {
public:
    Strategies() = default;
    virtual ~Strategies() { }

    virtual bool release_frame_after_capture() = 0;
    virtual bool release_frame_after_encode() = 0;
};

class MediaReceiverImpl;

class MediaReceiver {
public:
    MediaReceiver(
        const TransportInfo& info,
        std::unique_ptr<Strategies>&& strategies,
        std::unique_ptr<VideoDecoderInterface>&& decoder,
        std::unique_ptr<RenderInterface>&& render,
        std::shared_ptr<bco::Context> network_ctx,
        std::shared_ptr<bco::Context> jitter_ctx,
        std::shared_ptr<bco::Context> decode_ctx,
        std::shared_ptr<bco::Context> render_ctx);
    void start();
    void stop();

private:
    std::shared_ptr<MediaReceiverImpl> impl_;
};

class MediaSenderImpl;

class MediaSender  {
public:
    MediaSender(
        const TransportInfo& info,
        std::unique_ptr<Strategies>&& strategies,
        std::unique_ptr<VideoEncoderInterface>&& encoder,
        std::unique_ptr<VideoCaptureInterface>&& capture,
        std::shared_ptr<bco::Context> network_ctx,
        std::shared_ptr<bco::Context> encode_ctx,
        std::shared_ptr<bco::Context> pacer_ctx);
    void start();
    void stop();

private:
    std::shared_ptr<MediaSenderImpl> impl_;
};

} // namespace brtc

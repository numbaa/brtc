#pragma once
#include <memory>
#include <deque>
#include <atomic>
#include <brtc/interface.h>
#include <bco/coroutine/channel.h>
#include <bco/net/proactor/select.h>
#include <bco/context.h>
#include "transport/transport.h"
#include "video/frame_assembler/frame_assembler.h"
#include "video/frame_buffer/frame_buffer.h"
#include "video/reference_finder/reference_finder.h"

namespace brtc {

class MediaReceiverImpl : public std::enable_shared_from_this<MediaReceiverImpl> {
public:
    MediaReceiverImpl(
        std::unique_ptr<Transport>&& transport,
        std::unique_ptr<VideoDecoderInterface>&& decoder,
        std::unique_ptr<RenderInterface>&& render,
        std::shared_ptr<bco::Context<bco::net::Select>> network_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> decode_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> render_ctx);
    void start();
    void stop();

private:
    bco::Routine network_loop(std::shared_ptr<MediaReceiverImpl> that);
    bco::Routine decode_loop(std::shared_ptr<MediaReceiverImpl> that);
    bco::Routine render_loop(std::shared_ptr<MediaReceiverImpl> that);
    inline void send_to_decode_loop(Frame frame);
    inline void send_to_render_loop(Frame frame);
    inline bco::Task<Frame> receive_from_network_loop();
    inline bco::Task<Frame> receive_from_decode_loop();
    Frame decode_one_frame(Frame frame);
    void render_one_frame(Frame frame);

private:
    std::atomic<bool> stop_ { true };
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<VideoDecoderInterface> decoder_;
    std::unique_ptr<RenderInterface> render_;
    std::shared_ptr<bco::Context<bco::net::Select>> network_ctx_;
    std::shared_ptr<bco::Context<bco::net::Select>> decode_ctx_;
    std::shared_ptr<bco::Context<bco::net::Select>> render_ctx_;
    FrameAssembler frame_assembler_;
    FrameBuffer frame_buffer_;
    RtpFrameReferenceFinder reference_finder_;
    bco::Channel<Frame> undecoded_frames_;
    bco::Channel<Frame> decoded_frames_;
};

} // namespace brtc
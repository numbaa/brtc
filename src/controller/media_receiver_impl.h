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
#include "video/jitter_buffer/jitter_buffer.h"
#include "video/reference_finder/reference_finder.h"

namespace brtc {

class MediaReceiverImpl : public std::enable_shared_from_this<MediaReceiverImpl> {
public:
    MediaReceiverImpl(
        const TransportInfo& info,
        std::unique_ptr<Strategies>&& strategies,
        std::unique_ptr<VideoDecoderInterface>&& decoder,
        std::unique_ptr<RenderInterface>&& render,
        std::shared_ptr<bco::Context> network_ctx,
        std::shared_ptr<bco::Context> decode_ctx,
        std::shared_ptr<bco::Context> render_ctx);
    void start();
    void stop();

private:
    bco::Routine network_loop(std::shared_ptr<MediaReceiverImpl> that);
    bco::Routine decode_loop(std::shared_ptr<MediaReceiverImpl> that);
    bco::Routine render_loop(std::shared_ptr<MediaReceiverImpl> that);
    bco::Routine jitter_loop(std::shared_ptr<MediaReceiverImpl> that);
    inline void send_to_decode_loop(Frame frame);
    inline void send_to_render_loop(Frame frame);
    inline bco::Task<Frame> receive_from_network_loop();
    inline bco::Task<Frame> receive_from_decode_loop();
    Frame decode_one_frame(Frame frame);
    void render_one_frame(Frame frame);
    void parse_rtp_extensions(RtpPacket& packet);

private:
    std::atomic<bool> stop_ { false };
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<Strategies> strategies_;
    std::unique_ptr<VideoDecoderInterface> decoder_;
    std::unique_ptr<RenderInterface> render_;
    std::shared_ptr<bco::Context> network_ctx_;
    std::shared_ptr<bco::Context> decode_ctx_;
    std::shared_ptr<bco::Context> render_ctx_;
    FrameAssembler frame_assembler_;
    JitterBuffer frame_buffer_;
    RtpFrameReferenceFinder reference_finder_;
    bco::Channel<Frame> undecoded_frames_;
    bco::Channel<Frame> decoded_frames_;
    webrtc::SeqNumUnwrapper<uint16_t> frame_id_unwrapper_;
};

} // namespace brtc
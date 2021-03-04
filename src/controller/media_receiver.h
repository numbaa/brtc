#pragma once
#include <memory>
#include <brtc/fwd.h>
#include <bco/bco.h>
#include <bco/net/proactor/select.h>
#include "../transport/transport.h"
#include "../frame_assembler/frame_assembler.h"
#include "../frame_buffer/frame_buffer.h"

namespace brtc {

class MediaReceiver : public std::enable_shared_from_this<MediaReceiver> {
public:
    MediaReceiver();
    void start();
    void set_executor(); // or config in ctor
    void set_transport(std::unique_ptr<Transport>&& transport);
    void set_decoder(std::unique_ptr<VideoDecoderInterface>&& decoder);
    void set_render(std::unique_ptr<RenderInterface>&& render);

private:
    bco::Task<> network_loop(std::shared_ptr<MediaReceiver> that);
    bco::Task<> decode_loop(std::shared_ptr<MediaReceiver> that);
    bco::Task<> render_loop(std::shared_ptr<MediaReceiver> that);
    void send_to_decode_loop(Frame frame);
    void send_to_render_loop(Frame frame);
    bco::Task<Frame> receive_from_network_loop();
    bco::Task<Frame> receive_from_decode_loop();
    Frame decode_one_frame(Frame frame);
    void render_one_frame(Frame frame);

private:
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<VideoDecoderInterface> decoder_;
    std::unique_ptr<RenderInterface> render_;
    std::shared_ptr<bco::Context<bco::net::Select>> network_ctx_;
    std::shared_ptr<bco::Context<bco::net::Select>> decode_ctx_;
    std::shared_ptr<bco::Context<bco::net::Select>> render_ctx_;
    FrameAssembler frame_assembler_;
    FrameBuffer frame_buffer_;
};

} // namespace brtc
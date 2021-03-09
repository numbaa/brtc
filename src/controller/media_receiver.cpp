#include "media_receiver.h"

namespace brtc {


MediaReceiver::MediaReceiver(
                std::unique_ptr<Transport>&& transport,
                std::unique_ptr<VideoDecoderInterface>&& decoder,
                std::unique_ptr<RenderInterface>&& render,
                std::shared_ptr<bco::Context<bco::net::Select>> network_ctx,
                std::shared_ptr<bco::Context<bco::net::Select>> decode_ctx,
                std::shared_ptr<bco::Context<bco::net::Select>> render_ctx)
    : transport_(std::move(transport))
    , decoder_(std::move(decoder))
    , render_(std::move(render))
    , network_ctx_(network_ctx)
    , decode_ctx_(decode_ctx)
    , render_ctx_(render_ctx)
{
}

void MediaReceiver::start()
{
    network_ctx_->spawn(std::bind(&MediaReceiver::network_loop, this, shared_from_this()));
    decode_ctx_->spawn(std::bind(&MediaReceiver::decode_loop, this, shared_from_this()));
    render_ctx_->spawn(std::bind(&MediaReceiver::render_loop, this, shared_from_this()));
}

void MediaReceiver::stop()
{
    stop_ = true;
}

bco::Routine MediaReceiver::network_loop(std::shared_ptr<MediaReceiver> that)
{
    while (!stop_) {
        std::vector<std::byte> buffer(1500);
        auto bytes_read = co_await transport_->read_packet(buffer);
        frame_assembler_.insert(buffer);
        while (auto frame = frame_assembler_.pop_assembled_frame()) {
            frame_buffer_.insert(frame.value());
        }
        while (auto frame = frame_buffer_.pop_decodable_frame()) {
            send_to_decode_loop(frame.value());
        }
    }
}

bco::Routine MediaReceiver::decode_loop(std::shared_ptr<MediaReceiver> that)
{
    while (!stop_) {
        auto undecoded_frame = co_await receive_from_network_loop();
        auto decoded_frame = decode_one_frame(undecoded_frame);
        send_to_render_loop(decoded_frame);
    }
}

bco::Routine MediaReceiver::render_loop(std::shared_ptr<MediaReceiver> that)
{
    while (!stop_) {
        auto decoded_frame = co_await receive_from_decode_loop();
        render_one_frame(decoded_frame);
    }
}

void MediaReceiver::send_to_decode_loop(Frame frame)
{
    undecoded_frames_.send(frame);
}

void MediaReceiver::send_to_render_loop(Frame frame)
{
    decoded_frames_.send(frame);
}

bco::Task<Frame> MediaReceiver::receive_from_network_loop()
{
    return undecoded_frames_.recv();
}

bco::Task<Frame> MediaReceiver::receive_from_decode_loop()
{
    return decoded_frames_.recv();
}

Frame MediaReceiver::decode_one_frame(Frame frame)
{
    return decoder_->decode_one_frame(frame);
}

void MediaReceiver::render_one_frame(Frame frame)
{
    render_->render_one_frame(frame);
}

} // namespace brtc
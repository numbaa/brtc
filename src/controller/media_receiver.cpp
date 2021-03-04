#include "media_receiver.h"

namespace brtc {

MediaReceiver::MediaReceiver()
{
}

void MediaReceiver::start()
{
    network_ctx_->spawn(std::bind(&MediaReceiver::network_loop, this, shared_from_this()));
    decode_ctx_->spawn(std::bind(&MediaReceiver::decode_loop, this, shared_from_this()));
    render_ctx_->spawn(std::bind(&MediaReceiver::render_loop, this, shared_from_this()));
}

bco::Task<> MediaReceiver::network_loop(std::shared_ptr<MediaReceiver> that)
{
    while (true) {
        auto udp_packet = co_await transport_->read_packet();
        frame_assembler_.insert(udp_packet);
        while (auto frame = frame_assembler_.pop_assembled_frame()) {
            frame_buffer_.insert(frame.value());
        }
        while (auto frame = frame_buffer_.pop_decodable_frame()) {
            send_to_decode_loop(frame.value());
        }
    }
}

bco::Task<> MediaReceiver::decode_loop(std::shared_ptr<MediaReceiver> that)
{
    while (true) {
        auto undecoded_frame = co_await receive_from_network_loop();
        auto decoded_frame = decode_one_frame(undecoded_frame);
        send_to_render_loop(decoded_frame);
    }
}

bco::Task<> MediaReceiver::render_loop(std::shared_ptr<MediaReceiver> that)
{
    while (true) {
        auto decoded_frame = co_await receive_from_decode_loop();
        render_one_frame(decoded_frame);
    }
}

void MediaReceiver::send_to_decode_loop(Frame frame)
{
}

void MediaReceiver::send_to_render_loop(Frame frame)
{
}

bco::Task<Frame> MediaReceiver::receive_from_network_loop()
{
    return bco::Task<Frame>();
}

bco::Task<Frame> MediaReceiver::receive_from_decode_loop()
{
    return bco::Task<Frame>();
}

Frame MediaReceiver::decode_one_frame(Frame frame)
{
    return Frame();
}

void MediaReceiver::render_one_frame(Frame frame)
{
}

} // namespace brtc
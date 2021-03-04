#include "media_sender.h"
#include "../common/packetizer.h"

namespace brtc {

MediaSender::MediaSender()
{
}

void MediaSender::start()
{

    network_ctx_->spawn(std::bind(&MediaSender::network_loop, this, shared_from_this()));
    encode_ctx_->spawn(std::bind(&MediaSender::capture_encode_loop, this, shared_from_this()));
    pacer_ctx_->spawn(std::bind(&MediaSender::pacing_loop, this, shared_from_this()));
}

bco::Task<> MediaSender::network_loop(std::shared_ptr<MediaSender> that)
{
    while (true) {
        auto packet = co_await transport_->read_packet();
    }
}

bco::Task<> MediaSender::capture_encode_loop(std::shared_ptr<MediaSender> that)
{
    while (true) {
        auto raw_frame = co_await capture_one_frame();
        auto encoded_frame = co_await encode_one_frame(raw_frame);
        send_to_pacing_loop(encoded_frame);
    }
}

bco::Task<> MediaSender::pacing_loop(std::shared_ptr<MediaSender> that)
{
    while (true) {
        auto frame = co_await receive_from_encode_loop();
        std::vector<uint8_t> fake_frame; //暂时的
        Packetizer packetizer { fake_frame };
        RtpPacket packet;
        while (packetizer.next_packet(packet)) {
            //加策略delay一下啥的
            UdpPacket udp_packet;
            transport_->send_packet(udp_packet);
        }
    }
}


} // namespace brtc
#include "media_receiver.h"

namespace brtc {

MediaReceiver::MediaReceiver()
{
}

void MediaReceiver::start()
{
    while (true) {
        RtpPacket packet = transport_->read_packet();
        rtp_buffer_.push_back(packet);
        DePacketizer depkt;
        while (auto encoded_frame = depkt.depacketizer(rtp_buffer)) {
            frame_buffer_.push(encoded_frame);
        }
        for (auto encoded_frame : frame_buffer_) {
            Frame frame = decoder_->decode(encoded_frame);
            render_->render(frame);
        }
    }
}

} // namespace brtc
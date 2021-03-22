#include "media_sender_impl.h"
#include "../common/packetizer.h"

namespace brtc {

MediaSenderImpl::MediaSenderImpl(
            std::unique_ptr<Transport>&& transport,
            std::unique_ptr<VideoEncoderInterface>&& encoder,
            std::unique_ptr<VideoCaptureInterface>&& capture,
            std::shared_ptr<bco::Context<bco::net::Select>> network_ctx,
            std::shared_ptr<bco::Context<bco::net::Select>> encode_ctx,
            std::shared_ptr<bco::Context<bco::net::Select>> pacer_ctx)
    : transport_(std::move(transport))
    , encoder_(std::move(encoder))
    , capture_(std::move(capture))
    , network_ctx_(network_ctx)
    , encode_ctx_(encode_ctx)
    , pacer_ctx_(pacer_ctx)
{
}

void MediaSenderImpl::start()
{
    stop_ = false;
    network_ctx_->spawn(std::bind(&MediaSenderImpl::network_loop, this, shared_from_this()));
    encode_ctx_->spawn(std::bind(&MediaSenderImpl::capture_encode_loop, this, shared_from_this()));
    pacer_ctx_->spawn(std::bind(&MediaSenderImpl::pacing_loop, this, shared_from_this()));
}

void MediaSenderImpl::stop()
{
    stop_ = true;
}

bco::Routine MediaSenderImpl::network_loop(std::shared_ptr<MediaSenderImpl> that)
{
    //主要读rtcp
    while (!stop_) {
        bco::Buffer buffer(1500);
        auto bytes_read = co_await transport_->read_packet(buffer);
    }
}

bco::Routine MediaSenderImpl::capture_encode_loop(std::shared_ptr<MediaSenderImpl> that)
{
    while (!stop_) {
        auto raw_frame = capture_one_frame();
        auto encoded_frame = encode_one_frame(raw_frame);
        send_to_pacing_loop(encoded_frame);
    }
}

bco::Routine MediaSenderImpl::pacing_loop(std::shared_ptr<MediaSenderImpl> that)
{
    while (!stop_) {
        auto frame = co_await receive_from_encode_loop();
        Packetizer packetizer { frame };
        RtpPacket packet;
        while (packetizer.next_packet(packet)) {
            //加策略delay一下啥的
            transport_->send_packet(packet.data());
        }
    }
}

Frame MediaSenderImpl::capture_one_frame()
{
    return capture_->capture_one_frame();
}

Frame MediaSenderImpl::encode_one_frame(Frame frame)
{
    return encoder_->encode_one_frame(frame);
}

void MediaSenderImpl::send_to_pacing_loop(Frame frame)
{
    encoded_frames_.send(frame);
}

inline bco::Task<Frame> MediaSenderImpl::receive_from_encode_loop()
{
    return encoded_frames_.recv();
}


} // namespace brtc
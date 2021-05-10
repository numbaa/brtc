#include <vector>
#include <bco/coroutine/cofunc.h>
#include "media_sender_impl.h"
#include "../video/packetizer/packetizer.h"

namespace {
constexpr uint32_t kDefaultSsrc = 11223344;
constexpr uint32_t kDefaultPayloadType = 127;
}

namespace brtc {

MediaSenderImpl::MediaSenderImpl(
            const TransportInfo& info,
            std::unique_ptr<Strategies>&& strategies,
            std::unique_ptr<VideoEncoderInterface>&& encoder,
            std::unique_ptr<VideoCaptureInterface>&& capture,
            std::shared_ptr<bco::Context> network_ctx,
            std::shared_ptr<bco::Context> encode_ctx,
            std::shared_ptr<bco::Context> pacer_ctx)
    : transport_(std::make_unique<Transport>(network_ctx, info))
    , strategies_(std::move(strategies))
    , encoder_(std::move(encoder))
    , capture_(std::move(capture))
    , network_ctx_(network_ctx)
    , encode_ctx_(encode_ctx)
    , pacer_ctx_(pacer_ctx)
{
    start_timestamp_ = ::rand();
    seq_number_ = ::rand();
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
    while (!stop_) {
        auto packet = co_await transport_->recv_rtcp();
    }
}

bco::Routine MediaSenderImpl::capture_encode_loop(std::shared_ptr<MediaSenderImpl> that)
{
    while (!stop_) {
        auto raw_frame = capture_one_frame();
        after_capture();
        auto encoded_frame = encode_one_frame(raw_frame);
        after_encode();
        send_to_pacing_loop(encoded_frame);
        co_await bco::sleep_for(std::chrono::milliseconds { 16 });
    }
    co_return;
}

bco::Routine MediaSenderImpl::pacing_loop(std::shared_ptr<MediaSenderImpl> that)
{
    while (!stop_) {
        auto frame = co_await receive_from_encode_loop();
        Packetizer::PayloadSizeLimits limits;
        std::unique_ptr<Packetizer> packetizer = Packetizer::create(frame, VideoCodecType::H264, limits);
        RtpPacket packet;
        while (packetizer->has_next_packet()) {
            RtpPacket packet;
            packet.set_ssrc(kDefaultSsrc);
            packet.set_payload_type(kDefaultPayloadType);
            packet.set_timestamp(frame.timestamp + start_timestamp_);
            packet.set_sequence_number(seq_number_++);
            //first packet of frame
            //allow retransmission
            //is key frame
            //packet type
            packetizer->next_packet(packet);
            transport_->send_rtp(packet);
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

void MediaSenderImpl::after_capture()
{
    if (strategies_->release_frame_after_capture()) {
        capture_->release_frame();
    }
}

void MediaSenderImpl::after_encode()
{
    if (strategies_->release_frame_after_encode()) {
        capture_->release_frame();
    }
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
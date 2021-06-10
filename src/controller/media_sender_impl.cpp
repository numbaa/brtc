#include <vector>
#include <bco/coroutine/cofunc.h>
#include "media_sender_impl.h"
#include "../video/packetizer/packetizer.h"

namespace {
constexpr uint32_t kDefaultSsrc = 11223344;
constexpr uint32_t kDefaultPayloadType = 127;
}

namespace brtc {

class _fake_cfg : public webrtc::WebRtcKeyValueConfig {
public:
    std::string Lookup(std::string_view key) const { return ""; }
};

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
    seq_number_ = static_cast<uint16_t>(::rand());
    params_.push_back(RtpPayloadParams(kDefaultSsrc, nullptr, _fake_cfg {}));
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
        co_await bco::sleep_for(std::chrono::milliseconds { 16 });
        auto raw_frame = capture_one_frame();
        if (raw_frame.data == nullptr) {
            capture_empty_frame();
            continue;
        }
        after_capture();
        auto encoded_frame = encode_one_frame(raw_frame);
        if (encoded_frame.data == nullptr) {
            encode_failed();
            continue;
        }
        after_encode();
        send_to_pacing_loop(encoded_frame);
    }
    co_return;
}

bco::Routine MediaSenderImpl::pacing_loop(std::shared_ptr<MediaSenderImpl> that)
{
    while (!stop_) {
        auto frame = co_await receive_from_encode_loop();
        shared_frame_id_++;
        Packetizer::PayloadSizeLimits limits;
        std::unique_ptr<Packetizer> packetizer = Packetizer::create(frame, VideoCodecType::H264, limits);

        frame.video_frame_type = packetizer->is_key_frame() ? VideoFrameType::VideoFrameKey : VideoFrameType::VideoFrameDelta;
        auto video_header = get_rtp_video_header(frame);

        size_t num_packets = packetizer->num_packets();
        for (size_t i = 0; i < num_packets; i++) {
            const bool is_first_packet = i == 0;
            const bool is_last_packet = i == num_packets - 1;
            RtpPacket packet;
            packet.set_ssrc(kDefaultSsrc);
            packet.set_payload_type(kDefaultPayloadType);
            packet.set_timestamp(frame.timestamp + start_timestamp_);
            packet.set_sequence_number(seq_number_++);
            //first packet of frame
            //allow retransmission
            //is key frame
            //packet type
            add_required_rtp_extensions(packet, video_header, is_first_packet, is_last_packet);
            packetizer->next_packet(packet);
            transport_->send_rtp(packet);
        }
    }
}

Frame MediaSenderImpl::capture_one_frame()
{
    return capture_->capture_one_frame();
}

EncodedFrame MediaSenderImpl::encode_one_frame(Frame frame)
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

void MediaSenderImpl::capture_empty_frame()
{
    if (strategies_->release_frame_after_capture()) {
        capture_->release_frame();
    }
}

void MediaSenderImpl::encode_failed()
{
    if (strategies_->release_frame_after_encode()) {
        capture_->release_frame();
    }
}

void MediaSenderImpl::send_to_pacing_loop(EncodedFrame frame)
{
    encoded_frames_.send(frame);
}

inline bco::Task<EncodedFrame> MediaSenderImpl::receive_from_encode_loop()
{
    return encoded_frames_.recv();
}

void MediaSenderImpl::add_required_rtp_extensions(RtpPacket& packet, const RTPVideoHeader& rtp_video_header, bool is_first_packet, bool is_last_packet)
{
    //TODO: add more extensions
    RtpGenericFrameDescriptor descriptor;
    descriptor.SetFirstPacketInSubFrame(is_first_packet);
    descriptor.SetLastPacketInSubFrame(is_last_packet);
    if (is_first_packet) {
        descriptor.SetFrameId(static_cast<uint16_t>(rtp_video_header.generic->frame_id));
        for (int64_t dep : rtp_video_header.generic->dependencies) {
            descriptor.AddFrameDependencyDiff(static_cast<uint16_t>(rtp_video_header.generic->frame_id - dep));
        }

        uint8_t spatial_bimask = 1 << rtp_video_header.generic->spatial_index;
        descriptor.SetSpatialLayersBitmask(spatial_bimask);

        descriptor.SetTemporalLayer(static_cast<uint8_t>(rtp_video_header.generic->temporal_index));

        if (rtp_video_header.frame_type == VideoFrameType::VideoFrameKey) {
            descriptor.SetResolution(rtp_video_header.width, rtp_video_header.height);
        }
    }
    packet.set_extension<RtpGenericFrameDescriptorExtension00>(descriptor);
}

RTPVideoHeader MediaSenderImpl::get_rtp_video_header(EncodedFrame& frame)
{
    // TODO: Not only get rtp video header
    size_t stream_index = 0;
    if (frame.codec_info.has_value() && (frame.codec_info->codecType == VideoCodecType::VP8 || frame.codec_info->codecType == VideoCodecType::H264 || frame.codec_info->codecType == VideoCodecType::Generic)) {
        // Map spatial index to simulcast.
        stream_index = frame.spatial_index.value_or(0);
    }
    return params_[stream_index].GetRtpVideoHeader(frame, frame.codec_info.has_value() ? &frame.codec_info.value() : nullptr, shared_frame_id_);
}


} // namespace brtc
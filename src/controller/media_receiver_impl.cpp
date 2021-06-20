#include "rtp/extension.h"
#include "media_receiver_impl.h"
#include <glog/logging.h>

namespace brtc {

namespace {
constexpr size_t kStartPacketBufferSize = 512;
constexpr size_t kMaxPacketBufferSize = 1000;
constexpr size_t kDecodedHistorySize = 1000;
}


MediaReceiverImpl::MediaReceiverImpl(
                const TransportInfo& info,
                std::unique_ptr<Strategies>&& strategies,
                std::unique_ptr<VideoDecoderInterface>&& decoder,
                std::unique_ptr<RenderInterface>&& render,
                std::shared_ptr<bco::Context> network_ctx,
                std::shared_ptr<bco::Context> jitter_ctx,
                std::shared_ptr<bco::Context> decode_ctx,
                std::shared_ptr<bco::Context> render_ctx)
    : transport_(std::make_unique<Transport>(network_ctx, info))
    , strategies_(std::move(strategies))
    , decoder_(std::move(decoder))
    , render_(std::move(render))
    , network_ctx_(network_ctx)
    , decode_ctx_(decode_ctx)
    , render_ctx_(render_ctx)
    , frame_assembler_(kStartPacketBufferSize, kMaxPacketBufferSize)
    , jitter_buffer_(kDecodedHistorySize, jitter_ctx)
{
}

void MediaReceiverImpl::start()
{
    network_ctx_->spawn(std::bind(&MediaReceiverImpl::network_loop, this, shared_from_this()));
    decode_ctx_->spawn(std::bind(&MediaReceiverImpl::decode_loop, this, shared_from_this()));
    render_ctx_->spawn(std::bind(&MediaReceiverImpl::render_loop, this, shared_from_this()));
    jitter_buffer_.start();
}

void MediaReceiverImpl::stop()
{
    stop_ = true;
    jitter_buffer_.stop();
}

bco::Routine MediaReceiverImpl::network_loop(std::shared_ptr<MediaReceiverImpl> that)
{
    while (!stop_) {
        auto packet = co_await transport_->recv_rtp();
        parse_video_header(packet);
        parse_rtp_extensions(packet);
        frame_assembler_.insert(packet);
        while (auto frame = frame_assembler_.pop_assembled_frame()) {
            reference_finder_.ManageFrame(std::move(frame));
        }
        while (auto frame = reference_finder_.pop_gop_inter_continous_frame()) {
            jitter_buffer_.insert(*frame);
        }
    }
}

bco::Routine MediaReceiverImpl::decode_loop(std::shared_ptr<MediaReceiverImpl> that)
{
    while (!stop_) {
        auto undecoded_frame = co_await jitter_buffer_.pop_decodable_frame();
        auto decoded_frame = decode_one_frame(undecoded_frame);
        send_to_render_loop(decoded_frame);
    }
}

bco::Routine MediaReceiverImpl::render_loop(std::shared_ptr<MediaReceiverImpl> that)
{
    while (!stop_) {
        auto decoded_frame = co_await receive_from_decode_loop();
        render_one_frame(decoded_frame);
    }
}

void MediaReceiverImpl::send_to_decode_loop(Frame frame)
{
    undecoded_frames_.send(frame);
}

void MediaReceiverImpl::send_to_render_loop(Frame frame)
{
    decoded_frames_.send(frame);
}

bco::Task<Frame> MediaReceiverImpl::receive_from_network_loop()
{
    return undecoded_frames_.recv();
}

bco::Task<Frame> MediaReceiverImpl::receive_from_decode_loop()
{
    return decoded_frames_.recv();
}

Frame MediaReceiverImpl::decode_one_frame(Frame frame)
{
    return decoder_->decode_one_frame(frame);
}

void MediaReceiverImpl::render_one_frame(Frame frame)
{
    render_->render_one_frame(frame);
}

void MediaReceiverImpl::parse_rtp_extensions(RtpPacket& packet)
{
    RtpGenericFrameDescriptor descriptor;
    if (packet.get_extension<RtpGenericFrameDescriptorExtension00>(descriptor)) {
        auto& video_header = packet.video_header();
        video_header.is_first_packet_in_frame = descriptor.FirstPacketInSubFrame();
        video_header.is_last_packet_in_frame = descriptor.LastPacketInSubFrame();
        if (descriptor.FirstPacketInSubFrame()) {
            video_header.frame_type = descriptor.FrameDependenciesDiffs().empty()
                ? VideoFrameType::VideoFrameKey
                : VideoFrameType::VideoFrameDelta;

            auto& generic_descriptor_info = video_header.generic.emplace();
            int64_t frame_id = frame_id_unwrapper_.Unwrap(descriptor.FrameId());
            generic_descriptor_info.frame_id = frame_id;
            generic_descriptor_info.spatial_index = descriptor.SpatialLayer();
            generic_descriptor_info.temporal_index = descriptor.TemporalLayer();
            for (uint16_t fdiff : descriptor.FrameDependenciesDiffs()) {
                generic_descriptor_info.dependencies.push_back(frame_id - fdiff);
            }
        }
        video_header.width = static_cast<uint16_t>(descriptor.Width());
        video_header.height = static_cast<uint16_t>(descriptor.Height());
        //emmmmm....
        video_header.codec = VideoCodecType::H264;
    }
}

void MediaReceiverImpl::parse_video_header(RtpPacket& packet)
{
    //
}

} // namespace brtc
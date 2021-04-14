#include "media_receiver_impl.h"

namespace brtc {

namespace {
constexpr size_t kMaxPacketBufferSize = 1000;
constexpr size_t kDecodedHistorySize = 1000;
}


MediaReceiverImpl::MediaReceiverImpl(
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
    , frame_assembler_(kMaxPacketBufferSize)
    , frame_buffer_(kDecodedHistorySize)
{
}

void MediaReceiverImpl::start()
{
    network_ctx_->spawn(std::bind(&MediaReceiverImpl::network_loop, this, shared_from_this()));
    decode_ctx_->spawn(std::bind(&MediaReceiverImpl::decode_loop, this, shared_from_this()));
    render_ctx_->spawn(std::bind(&MediaReceiverImpl::render_loop, this, shared_from_this()));
}

void MediaReceiverImpl::stop()
{
    stop_ = true;
}

bco::Routine MediaReceiverImpl::network_loop(std::shared_ptr<MediaReceiverImpl> that)
{
    while (!stop_) {
        auto packet = co_await transport_->recv_rtp();
        frame_assembler_.insert(packet);
        while (auto frame = frame_assembler_.pop_assembled_frame()) {
            //TODO: Frame -> ReceivedFrame
            std::unique_ptr<ReceivedFrame> received_frame;
            reference_finder_.ManageFrame(std::move(received_frame));
        }
        //jitter的处理也先不考虑，得到一个可解码帧就扔去解码，jitter另外做，要做在controller里
        while (auto frame = reference_finder_.pop_gop_inter_continous_frame()) {
            frame_buffer_.insert(*frame);
        }
        while (auto frame = frame_buffer_.pop_decodable_frame()) {
            send_to_decode_loop(frame.value());
        }
    }
}

bco::Routine MediaReceiverImpl::decode_loop(std::shared_ptr<MediaReceiverImpl> that)
{
    while (!stop_) {
        auto undecoded_frame = co_await receive_from_network_loop();
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

} // namespace brtc
#pragma once
#include <memory>
#include <atomic>

#include <brtc/interface.h>
#include <bco/coroutine/channel.h>
#include <bco/net/proactor/select.h>
#include <bco/context.h>
#include "../transport/transport.h"

namespace brtc {

class MediaSenderImpl : public std::enable_shared_from_this<MediaSenderImpl> {
public:
    MediaSenderImpl(
        const TransportInfo& info,
        std::unique_ptr<Strategies>&& strategies,
        std::unique_ptr<VideoEncoderInterface>&& encoder,
        std::unique_ptr<VideoCaptureInterface>&& capture,
        std::shared_ptr<bco::Context> network_ctx,
        std::shared_ptr<bco::Context> encode_ctx,
        std::shared_ptr<bco::Context> pacer_ctx);
    void start();
    void stop();

private:
    bco::Routine network_loop(std::shared_ptr<MediaSenderImpl> that);
    bco::Routine capture_encode_loop(std::shared_ptr<MediaSenderImpl> that);
    bco::Routine pacing_loop(std::shared_ptr<MediaSenderImpl> that);

    Frame capture_one_frame();
    Frame encode_one_frame(Frame frame);

    void after_capture();
    void after_encode();
    void capture_empty_frame();
    void encode_failed();

    inline void send_to_pacing_loop(Frame frame);
    inline bco::Task<Frame> receive_from_encode_loop();

    void add_required_rtp_extensions(RtpPacket& packet, bool is_first_packet, bool is_last_packet);

private:
    std::atomic<bool> stop_ { true };
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<Strategies> strategies_;
    std::unique_ptr<VideoEncoderInterface> encoder_;
    std::unique_ptr<VideoCaptureInterface> capture_;
    std::shared_ptr<bco::Context> network_ctx_;
    std::shared_ptr<bco::Context> encode_ctx_;
    std::shared_ptr<bco::Context> pacer_ctx_;
    bco::Channel<Frame> encoded_frames_;
    uint32_t start_timestamp_;
    uint16_t seq_number_;
};

} // namespace brtc
#pragma once
#include <memory>

#include <brtc/fwd.h>
#include <bco/net/proactor/select.h>
#include "../transport/transport.h"

namespace brtc {

class MediaSender : public std::enable_shared_from_this<MediaSender> {
public:
    MediaSender();
    void start();
    void set_executor(); //or config in ctor
    void set_transport(std::unique_ptr<Transport>&& transport);
    void set_encoder(std::unique_ptr<VideoEncoderInterface>&& encoder);
    void set_capture(std::unique_ptr<VideoCaptureInterface>&& capture);

    void on_raw_frame();
    void on_encoded_frame();
    void on_packet();
    void on_sendable_packet();

private:
    bco::Task<> network_loop(std::shared_ptr<MediaSender> that);
    bco::Task<> capture_encode_loop(std::shared_ptr<MediaSender> that);
    bco::Task<> pacing_loop(std::shared_ptr<MediaSender> that);

    bco::Task<Frame> capture_one_frame();
    bco::Task<Frame> encode_one_frame(Frame frame);

    void send_to_pacing_loop(Frame frame);
    bco::Task<Frame> receive_from_encode_loop();

private:
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<VideoEncoderInterface> encoder_;
    std::unique_ptr<VideoCaptureInterface> capture_;
    std::shared_ptr<bco::Context<bco::net::Select>> network_ctx_;
    std::shared_ptr<bco::Context<bco::net::Select>> encode_ctx_;
    std::shared_ptr<bco::Context<bco::net::Select>> pacer_ctx_;
};

} // namespace brtc
#pragma once
#include <memory>

#include <brtc/fwd.h>
#include "../common/transport.h"

namespace brtc {

class MediaSender {
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
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<VideoEncoderInterface> encoder_;
    std::unique_ptr<VideoCaptureInterface> capture_;
};

} // namespace brtc
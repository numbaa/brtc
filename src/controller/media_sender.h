#pragma once
#include <memory>
#include "../common/transport.h"
#include "../common/encoder.h"
#include "../common/capture.h"

namespace brtc {

class MediaSender {
public:
    MediaSender();
    void set_transport(std::unique_ptr<Transport>&& transport);
    void set_encoder(std::unique_ptr<VideoEncoderInterface>&& encoder);
    void set_capture(std::unique_ptr<VideoCaptureInterface>&& capture);

private:
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<VideoEncoderInterface> encoder_;
    std::unique_ptr<VideoCaptureInterface> capture_;
};

} // namespace brtc
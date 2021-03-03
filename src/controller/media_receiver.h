#pragma once
#include <memory>
#include <brtc/fwd.h>
#include "../common/transport.h"

namespace brtc {

class MediaReceiver {
public:
    MediaReceiver();
    void start();
    void set_transport(std::unique_ptr<Transport>&& transport);
    void set_decoder(std::unique_ptr<VideoDecoderInterface>&& decoder);
    void set_render(std::unique_ptr<RenderInterface>&& render);

private:
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<VideoDecoderInterface> decoder_;
    std::unique_ptr<RenderInterface> render_;
};

} // namespace brtc
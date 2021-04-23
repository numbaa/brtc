#include <brtc/interface.h>
#include "transport/transport.h"
#include "controller/media_sender_impl.h"

namespace brtc {

MediaSender::MediaSender(
        const TransportInfo& info,
        std::unique_ptr<VideoEncoderInterface>&& encoder,
        std::unique_ptr<VideoCaptureInterface>&& capture,
        std::shared_ptr<bco::Context> network_ctx,
        std::shared_ptr<bco::Context> encode_ctx,
        std::shared_ptr<bco::Context> pacer_ctx)
    : impl_ { std::make_shared<MediaSenderImpl>(
        info,
        std::move(encoder),
        std::move(capture),
        network_ctx,
        encode_ctx,
        pacer_ctx) }
{
}

void MediaSender::start()
{
    impl_->start();
}

void MediaSender::stop()
{
    impl_->stop();
}

} // namespace brtc

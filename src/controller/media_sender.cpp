#include <brtc/interface.h>
#include "../transport/transport.h"
#include "media_sender_impl.h"

namespace brtc {

MediaSender::MediaSender(
        std::unique_ptr<VideoEncoderInterface>&& encoder,
        std::unique_ptr<VideoCaptureInterface>&& capture,
        std::shared_ptr<bco::Context<bco::net::Select>> network_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> encode_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> pacer_ctx)
    : impl_ { std::make_shared<MediaSenderImpl>(
        new Transport {},
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

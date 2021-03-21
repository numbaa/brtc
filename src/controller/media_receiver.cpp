#include <brtc/interface.h>
#include "../transport/transport.h"
#include "media_receiver_impl.h"

namespace brtc {

MediaReceiver::MediaReceiver(
        std::unique_ptr<VideoDecoderInterface>&& decoder,
        std::unique_ptr<RenderInterface>&& render,
        std::shared_ptr<bco::Context<bco::net::Select>> network_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> decode_ctx,
        std::shared_ptr<bco::Context<bco::net::Select>> render_ctx)
    : impl_ { std::make_shared<MediaReceiverImpl>(
        new Transport {},
        std::move(decoder),
        std::move(render),
        network_ctx,
        decode_ctx,
        render_ctx) }
{
}

void MediaReceiver::start()
{
    impl_->start();
}

void MediaReceiver::stop()
{
    impl_->stop();
}

} // namespace brtc

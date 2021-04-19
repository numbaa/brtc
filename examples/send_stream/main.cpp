#include <wrl/client.h>
#include <d3d11.h>

#include <memory>

#include <bco.h>

#include <brtc.h>
#include <brtc/builtin.h>

class SendStream {
    using Context = bco::Context<bco::net::Select>;
public:
    SendStream(const brtc::TransportInfo& transport_info, Microsoft::WRL::ComPtr<ID3D11Device> device);
    void start();

private:
    std::shared_ptr<Context> context_;
    brtc::MediaSender sender_;
};

SendStream::SendStream(const brtc::TransportInfo& transport_info, Microsoft::WRL::ComPtr<ID3D11Device> device)
    : context_(std::make_shared<Context>(std::make_unique<bco::SimpleExecutor>()))
    , sender_(
        transport_info,
        brtc::builtin::create_encoder_mfx(device),
        brtc::builtin::create_capture_dxgi(device),
        context_, context_, context_)
{
}

void SendStream::start()
{
    context_->start();
    sender_.start();
}

int main()
{
    brtc::TransportInfo transport_info;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    SendStream stream { transport_info, device };
    stream.start();

    return 0;
}
#include <d3d11.h>
#include <wrl/client.h>

#include <memory>

#include <bco.h>

#include <brtc.h>
#include <brtc/builtin.h>

class RecvStream {
    using Context = bco::Context<bco::net::Select>;

public:
    RecvStream(const brtc::TransportInfo& transport_info, Microsoft::WRL::ComPtr<ID3D11Device> device);
    void start();

private:
    std::shared_ptr<Context> context_;
    brtc::MediaReceiver receiver_;
};

RecvStream::RecvStream(const brtc::TransportInfo& transport_info, Microsoft::WRL::ComPtr<ID3D11Device> device)
    : context_(std::make_shared<Context>(std::make_unique<bco::SimpleExecutor>()))
    , receiver_(
          transport_info,
          brtc::builtin::create_decoder_mfx(device),
          brtc::builtin::create_render_d3d11(device),
          context_, context_, context_)
{
}

void RecvStream::start()
{
    context_->start();
    receiver_.start();
}

int main()
{
    brtc::TransportInfo transport_info;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    RecvStream stream { transport_info, device };
    stream.start();

    return 0;
}
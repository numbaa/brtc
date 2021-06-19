#include <d3d11.h>
#include <wrl/client.h>

#include <memory>

#include <bco.h>
#include <glog/logging.h>

#include <brtc.h>
#include <brtc/builtin.h>

#include "../common/d3d_helper.h"

using UdpSocket = bco::net::UdpSocket<bco::net::Select>;

class RecvStream {
public:
    RecvStream(std::unique_ptr <bco::net::Select>&& proactor, const brtc::TransportInfo & transport_info, Microsoft::WRL::ComPtr<ID3D11Device> device);
    void start();

private:
    std::shared_ptr<bco::Context> context_;
    brtc::MediaReceiver receiver_;
};

RecvStream::RecvStream(std::unique_ptr<bco::net::Select>&& proactor, const brtc::TransportInfo& transport_info, Microsoft::WRL::ComPtr<ID3D11Device> device)
    : context_(std::make_shared<bco::Context>(std::make_unique<bco::SimpleExecutor>()))
    , receiver_(
          transport_info,
          brtc::builtin::create_strategies(1),
          brtc::builtin::create_decoder_mfx(device),
          brtc::builtin::create_render_d3d11(device),
          context_, context_, context_, context_)
{
    context_->add_proactor(std::move(proactor));
}

void RecvStream::start()
{
    context_->start();
    receiver_.start();
}

void init_winsock()
{
    WSADATA wsdata;
    (void)WSAStartup(MAKEWORD(2, 2), &wsdata);
}

int main()
{
    init_winsock();
    auto device = get_d3d11_device(GpuVendor::Intel);
    if (device == nullptr) {
        LOG(ERROR) << "get d3d11 device failed";
        return -1;
    }
    auto socket_proactor = std::make_unique<bco::net::Select>();
    socket_proactor->start(std::make_unique<bco::SimpleExecutor>());
    auto [sock, err] = UdpSocket::create(socket_proactor.get(), AF_INET);
    if (err != 0) {
        LOG(ERROR) << "create udp socket failed";
        return -1;
    }
    bco::net::Address local_addr { bco::net::IPv4 { "127.0.0.1" }, 43966 };
    sock.bind(local_addr);
    bco::net::Address remote_addr { bco::net::IPv4 { "127.0.0.1" }, 43967 };
    brtc::TransportInfo transport_info { .socket = sock, .remote_addr = remote_addr };
    RecvStream stream { std::move(socket_proactor), transport_info, device };
    stream.start();
    std::this_thread::sleep_for(std::chrono::seconds { 10000 });
    return 0;
}
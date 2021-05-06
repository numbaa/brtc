#include <bco/coroutine/cofunc.h>
#include "transport.h"

namespace brtc {

Transport::Transport(std::shared_ptr<bco::Context> ctx, const TransportInfo& info)
    : ctx_(ctx)
    , remote_addr_(info.remote_addr)
    , socket_(info.socket)
    , rtp_(new RtpTransport {std::bind(&Transport::send_packet, this, std::placeholders::_1)})
    , sctp_(new SctpTransport)
    , quic_(new QuicTransport)
{
    ctx_->spawn(std::bind(&Transport::recv_loop, this));
}

Transport::~Transport()
{

}

void Transport::set_socket(bco::net::UdpSocket<bco::net::Select> socket)
{
    socket_ = socket;
}

void Transport::set_remote_address(bco::net::Address addr)
{
    remote_addr_ = addr;
}

//bco::Func<bool> Transport::handshake(std::chrono::milliseconds timeout)
//{
//    auto result = co_await bco::run_with(bco::Timeout { timeout }, do_handshake());
//    if (result.has_value() && result.value() == true) {
//        co_return true;
//    }
//    //TODO: 记录一下握手失败还是握手超时
//    co_return false;
//}

bco::Task<RtpPacket> Transport::recv_rtp()
{
    return rtp_->recv_rtp_packet();
}

bco::Task<RtcpPacket> Transport::recv_rtcp()
{
    return rtp_->recv_rtcp_packet();
}

bco::Routine Transport::recv_loop()
{
    while (true) {
        bco::Buffer buff { 1500 };
        auto [bytes, addr] = co_await socket_.recvfrom(buff);
        std::apply([&buff](auto&... sink) { (..., sink->on_recv_data(buff)); },
            std::make_tuple(std::ref(rtp_), std::ref(sctp_), std::ref(quic_)));
    }
}

void Transport::send_packet(bco::Buffer packet)
{
    socket_.sendto(packet, remote_addr_);
}

//bco::Task<bool> Transport::do_handshake()
//{
//    //略麻烦
//    return bco::Task<bool>();
//}

void Transport::send_rtp(RtpPacket packet)
{
    rtp_->send_packet(packet);
}

void Transport::send_rtcp(RtcpPacket packet)
{
    rtp_->send_packet(packet);
}

} // namespace brtc

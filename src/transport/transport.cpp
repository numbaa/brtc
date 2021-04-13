#include "transport.h"

namespace brtc {

Transport::Transport()
{
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

bco::Task<RtpPacket> Transport::recv_rtp()
{
    return rtp_->recv_rtp_packet();
}

bco::Task<RtcpPacket> Transport::recv_rtcp()
{
    return rtp_->recv_rtcp_packet();
}

bco::Routine Transport::do_recv()
{
    bco::Buffer buff{1500};
    auto [bytes, addr] = co_await socket_.recvfrom(buff);
    std::apply([&buff](auto&... sink) { sink->on_recv_data(buff); },
               std::make_tuple(std::ref(rtp_), std::ref(sctp_), std::ref(quic_)));
}

void Transport::send_packet(bco::Buffer packet)
{
    socket_.sendto(packet, remote_addr_);
}

void Transport::send_rtp(RtpPacket packet)
{
    rtp_->send_packet(packet);
}

void Transport::send_rtcp(RtcpPacket packet)
{
    rtp_->send_packet(packet);
}

} // namespace brtc

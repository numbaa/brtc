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

void Transport::send_packet(bco::Buffer packet)
{
    socket_.sendto(packet, remote_addr_);
}

bco::Task<int> Transport::read_packet(bco::Buffer buffer)
{
    return socket_.recv(buffer);
}

} // namespace brtc

#pragma once

#include <cstdint>
#include <span>
#include <bco/buffer.h>
#include <bco/net/udp.h>
#include <bco/net/proactor/select.h>
#include "../common/udp_packet.h"

namespace brtc
{

//将来可以尝试提供
// read_rtp()、read_rtcp()、read_dtls()
//这类接口

class Transport {
public:
    Transport();
    ~Transport();
    void set_socket(bco::net::UdpSocket<bco::net::Select> socket);
    void set_remote_address(bco::net::Address addr);
    bco::Task<int> read_packet(bco::Buffer buffer);
    void send_packet(bco::Buffer packet);

private:
    bco::net::UdpSocket<bco::net::Select> socket_;
    bco::net::Address remote_addr_;
};

} // namespace brtc

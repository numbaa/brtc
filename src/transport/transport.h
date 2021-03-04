#pragma once

#include <cstdint>
#include <span>
#include <bco/bco.h>
#include "../common/udp_packet.h"

namespace brtc
{

    
class Transport {
public:
    Transport();
    ~Transport();
    bool init();
    bco::Task<UdpPacket> read_packet();
    void send_packet(UdpPacket packet);

private:
    int socket_;
};

} // namespace brtc

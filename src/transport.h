#pragma once

#include <cstdint>
#include <span>
#include "rtp.h"

namespace brtc
{

class TransportObserver {
public:
    virtual void on_rtp_packet(RtpPacket rtp) = 0;
    virtual void on_connected() = 0;
};
    
class Transport {
public:
    Transport();
    ~Transport();
    bool init();
    void set_observer(TransportObserver* observer);
    void send_packet(std::span<uint8_t> data);
private:
    int socket_;
    TransportObserver* observer_;
};

} // namespace brtc

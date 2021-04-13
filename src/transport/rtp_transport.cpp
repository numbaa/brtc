#include "transport/rtp_transport.h"

namespace brtc {

RtpTransport::RtpTransport(std::function<void(const bco::Buffer&)> send_func)
    : send_func_(send_func)
{
}

bco::Task<RtpPacket> RtpTransport::recv_rtp_packet()
{
    return rtp_packets_.recv();
}

bco::Task<RtcpPacket> RtpTransport::recv_rtcp_packet()
{
    return rtcp_packets_.recv();
}

void RtpTransport::send_packet(const RtpPacket& packet)
{
    send_func_(packet.data());
}

void RtpTransport::send_packet(const RtcpPacket& packet)
{
    //send_func_(packet.data());
}

void RtpTransport::on_recv_data(bco::Buffer buff)
{
    //parse Buffer -> RtpPacket
    constexpr int kRtp = 1;
    constexpr int kRtcp = 2;
    int type = 0;
    switch (type) {
    case kRtcp: {
        RtcpPacket packet;
        rtcp_packets_.send(packet);
        break;
    }
    case kRtp: {
        RtpPacket packet;
        rtp_packets_.send(packet);
        break;
    }
    default:
        break;
    }
}

} // namespace brtc

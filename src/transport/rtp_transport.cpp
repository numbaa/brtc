#include "transport/rtp_transport.h"

namespace brtc {

namespace {

enum class PacketType {
    Unknown,
    Rtp,
    Rtcp
};

constexpr uint8_t kRtpVersion = 2;
constexpr size_t kMinRtcpPacketLen = 4;
constexpr size_t kMinRtpPacketLen = 12;

bool is_rtp(const bco::Buffer& buff)
{
    //默认只有一个span
    auto data = buff.data();
    if (data.empty()) {
        return false;
    }
    auto buffer = data.front();
    if (buffer.size() < kMinRtpPacketLen) {
        return false;
    }
    return false;
}

bool is_rtcp(const bco::Buffer& buff)
{
    //默认只有一个span
    auto data = buff.data();
    if (data.empty()) {
        return false;
    }
    auto buffer = data.front();
    if (buffer.size() < kMinRtcpPacketLen) {
        return false;
    }
    if (buffer[0] >> 6 != kRtpVersion) {
        return false;
    }
    uint8_t pt = buffer[1] & 0x7F;
    return (63 < pt) && (pt < 96);
}

PacketType infer_packet_type(const bco::Buffer& buff)
{
    if (is_rtcp(buff)) {
        return PacketType::Rtcp;
    } else if (is_rtp(buff)) {
        return PacketType::Rtp;
    } else {
        return PacketType::Unknown;
    }
}

} // namespace

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

void RtpTransport::send_packet(const RtcpPacket&)
{
    //send_func_(packet.data());
}

void RtpTransport::on_recv_data(bco::Buffer buff)
{
    //parse Buffer -> RtpPacket
    PacketType type = PacketType::Unknown;
    switch (type) {
    case PacketType::Rtp: {
        RtcpPacket packet;
        rtcp_packets_.send(packet);
        break;
    }
    case PacketType::Rtcp: {
        RtpPacket packet { buff };
        //处理有问题的packet
        rtp_packets_.send(packet);
        break;
    }
    default:
        break;
    }
}

} // namespace brtc

#pragma once
#include <mutex>
#include <queue>

#include <bco/coroutine/task.h>
#include <bco/coroutine/channel.h>

#include "common/rtp.h"

namespace brtc {

class RtpTransport {
public:
    RtpTransport(std::function<void(const bco::Buffer&)> send_func);
    bco::Task<RtpPacket> recv_rtp_packet();
    bco::Task<RtcpPacket> recv_rtcp_packet();
    void send_packet(const RtpPacket& packet);
    void send_packet(const RtcpPacket& packet);
    void on_recv_data(bco::Buffer buff);

private:
    std::mutex mutex_;
    bco::Channel<RtpPacket> rtp_packets_;
    bco::Channel<RtcpPacket> rtcp_packets_;
    std::function<void(const bco::Buffer&)> send_func_;
};

} // namespace

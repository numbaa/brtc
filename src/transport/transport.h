#pragma once

#include <cstdint>
#include <span>
#include <bco/buffer.h>
#include <bco/net/udp.h>
#include <bco/net/proactor/select.h>
#include <brtc/interface.h>
#include "transport/rtp_transport.h"
#include "transport/sctp_transport.h"
#include "transport/quic_transport.h"
#include "common/rtp.h"

namespace brtc {

class Transport {
public:
    Transport(const TransportInfo& info);
    ~Transport();

    void set_socket(bco::net::UdpSocket<bco::net::Select> socket);
    void set_remote_address(bco::net::Address addr);

    //bco::Func<bool> handshake(std::chrono::milliseconds timeout);

    bco::Task<RtpPacket> recv_rtp();
    bco::Task<RtcpPacket> recv_rtcp();
    bco::Task<int> recv_sctp(bco::Buffer packet);
    bco::Task<int> recv_quic(bco::Buffer packet);

    void send_rtp(RtpPacket packet);
    void send_rtcp(RtcpPacket packet);
    void send_sctp(); // 这个可能需要返回bco::Task
    void send_quic(); // 这个可能需要返回bco::Task

private:
    bco::Routine do_recv();
    void send_packet(bco::Buffer packet);
    //bco::Task<bool> do_handshake();

private:
    bco::net::Address remote_addr_;
    bco::net::UdpSocket<bco::net::Select> socket_;
    std::unique_ptr<RtpTransport> rtp_;
    std::unique_ptr<SctpTransport> sctp_;
    std::unique_ptr<QuicTransport> quic_;
    std::atomic<bool> reading_ { false };
};

} // namespace brtc

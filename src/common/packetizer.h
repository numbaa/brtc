#pragma once
#include <cstdint>
#include <brtc/frame.h>
#include "rtp.h"

namespace brtc
{

class Packetizer {
public:
    explicit Packetizer(Frame decoded_frame);
    bool is_valid_frame() const;
    bool next_packet(RtpPacket& packet);
private:
    bool is_valid_frame_ = false;
    uint32_t current_nalu_ = 0;
    uint32_t nalus_len_ = 0;
    Frame frame_;
    struct StartCodeInfo {
        uint32_t start_code_length;
        uint32_t offset;
    };
    std::array<StartCodeInfo, kMaxNalusPerPacket> nalus_;
};

} // namespace brtc


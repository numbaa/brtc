#pragma once
#include <cstdint>
#include <span>
#include <array>
#include "rtp.h"

namespace brtc
{

class Packetizer {
public:
    explicit Packetizer(std::span<uint8_t> decoded_frame);
    bool is_valid_frame() const;
    bool next_packet(RtpPacket& packet);
private:
    bool is_valid_frame_ = false;
    uint32_t current_nalu_ = 0;
    uint32_t nalus_len_ = 0;
    std::span<uint8_t> frame_;
    struct StartCodeInfo {
        uint32_t start_code_length;
        uint32_t offset;
    };
    std::array<StartCodeInfo, kMaxNalusPerPacket> nalus_;
};

} // namespace brtc


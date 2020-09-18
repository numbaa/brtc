#pragma once
#include <cstdint>
#include <span>
#include "rtp.h"
namespace brtc
{

class Packetizer {
public:
    Packetizer(std::span<uint8_t> decoded_frame);
    bool next_frame(RtpPacket& packet);
private:
    uint32_t current_pos_;
};

} // namespace brtc


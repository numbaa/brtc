
#pragma once
#include <cstdint>
#include <array>
#include <queue>
#include <brtc/frame.h>
#include "../../common/rtp.h"

namespace brtc
{

class Packetizer {
public:
    bool is_valid_frame() const { return is_valid_frame_; }
    virtual bool next_packet(RtpPacket& packet) = 0;

protected:
    bool is_valid_frame_ = false;
};

} // namespace brtc


#pragma once
#include <optional>
#include <brtc/frame.h>
#include "../common/udp_packet.h"

namespace brtc {

class FrameAssembler {
public:
    void insert(UdpPacket packet);
    std::optional<Frame> pop_assembled_frame();
};

}
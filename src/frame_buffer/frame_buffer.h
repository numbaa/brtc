#pragma once
#include <optional>
#include <brtc/frame.h>

namespace brtc {

class FrameBuffer {
public:
    void insert(Frame frame);
    std::optional<Frame> pop_decodable_frame();
};

}
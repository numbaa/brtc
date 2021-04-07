#include "video/frame_buffer/frame_buffer.h"

namespace brtc {

void FrameBuffer::insert(Frame frame)
{
}

std::optional<Frame> FrameBuffer::pop_decodable_frame()
{
    return std::optional<Frame>();
}

} // namespace brtc

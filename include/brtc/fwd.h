#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include <brtc/frame.h>

namespace brtc
{
    
class VideoCaptureInterface {
public:
    virtual Frame capture_one_frame() = 0;
};

class VideoDecoderInterface {
public:
    virtual Frame decode_one_frame(Frame frame) = 0;
};

class VideoEncoderObserver {
public:
    virtual void on_encoded_frame(std::span<uint8_t> frame) = 0;
};

class VideoEncoderInterface {
public:
    virtual Frame encode_one_frame(Frame frame) = 0;
};


class RenderInterface {
public:
    virtual void render_one_frame(Frame frame) = 0;
};


} // namespace brtc

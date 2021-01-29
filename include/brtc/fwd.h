#pragma once
#include <cstdint>
#include <span>

#include <brtc/frame.h>

namespace brtc
{
    
class VideoCaptureInterface {
public:
    virtual Frame capture_one_frame() = 0;
};


class VideoDecoderObserver {
public:
    virtual void on_encoded_frame(std::span<uint8_t> frame) = 0;
};

class VideoDecoderInterface {
public:
    virtual void add_observer(VideoDecoderObserver* observer) = 0;
};

class VideoEncoderObserver {
public:
    virtual void on_encoded_frame(std::span<uint8_t> frame) = 0;
};

class VideoEncoderInterface {
public:
    virtual void add_observer(VideoEncoderObserver* observer) = 0;
    virtual std::vector<uint8_t> encode_one_frame(Frame frame) = 0;
};


class RenderInterface {
};


} // namespace brtc

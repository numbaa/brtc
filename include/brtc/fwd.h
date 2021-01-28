#pragma once
#include <span>

namespace brtc
{
    
class VideoCaptureInterface {
public:
    virtual void capture_one_frame();
};


class VideoDecoderObserver {
public:
    virtual void on_encoded_frame(std::span<uint8_t> frame);
};

class VideoDecoderInterface {
public:
    virtual void add_observer(VideoDecoderObserver* observer);
};

class VideoEncoderObserver {
public:
    virtual void on_encoded_frame(std::span<uint8_t> frame);
};

class VideoEncoderInterface {
public:
    virtual void add_observer(VideoEncoderObserver* observer);
};


class RenderInterface {
};


} // namespace brtc

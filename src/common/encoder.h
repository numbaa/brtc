#pragma once
#include <span>

namespace brtc {

class VideoEncoderObserver {
public:
    virtual void on_encoded_frame(std::span<uint8_t> frame);
};

class VideoEncoderInterface {
public:
    virtual void add_observer(VideoEncoderObserver* observer);
};

}
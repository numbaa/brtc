#pragma once
#include <span>

namespace brtc {

class VideoDecoderObserver {
public:
    virtual void on_encoded_frame(std::span<uint8_t> frame);
};

class VideoDecoderInterface {
public:
    virtual void add_observer(VideoDecoderObserver* observer);
};

}
#pragma once
#include <span>

namespace brtc {

class VideoSourceObserver {
public:
    virtual void on_raw_frame(std::span<uint8_t> frame);
};

class VideoSourceInterface {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void add_observer(VideoSourceObserver* observer) = 0;
};

}
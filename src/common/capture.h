#pragma once

namespace brtc {

class VideoCaptureInterface {
public:
    virtual void capture_one_frame();
};

} // namespace brtc
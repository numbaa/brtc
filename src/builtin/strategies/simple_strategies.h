#pragma once
#include <brtc/interface.h>

namespace brtc {

namespace builtin {

class Strategies1 : public Strategies {
public:
    Strategies1() = default;
    ~Strategies1() override { }

    bool release_frame_after_capture() override;
    bool release_frame_after_encode() override;
};

class Strategies2 : public Strategies {
public:
    Strategies2() = default;
    ~Strategies2() override { }

    bool release_frame_after_capture() override;
    bool release_frame_after_encode() override;
};

} // namespace builtin

} // namespace brtc
#include "simple_strategies.h"
#pragma once
#include "simple_strategies.h"

namespace brtc {

namespace builtin {

bool Strategies1::release_frame_after_capture()
{
    return false;
}

bool Strategies1::release_frame_after_encode()
{
    return true;
}

bool Strategies2::release_frame_after_capture()
{
    return true;
}

bool Strategies2::release_frame_after_encode()
{
    return false;
}

} // namespace builtin

} // namespace brtc
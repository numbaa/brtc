#pragma once
#include <cstdint>
#include <chrono>

namespace brtc {

inline int64_t MachineNowMilliseconds()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
}

inline int64_t MachineNowMicroseconds()
{
    return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
}

inline int64_t UTCNowMilliseconds()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
}

inline int64_t UTCNowMicroseconds()
{
    return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
}

} // namespace brtc

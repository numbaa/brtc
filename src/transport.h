#pragma once

#include <cstdint>
#include <span>

namespace brtc
{
    
class Transport {
public:
    Transport();
    void send_packet(std::span<uint8_t> data);
};

} // namespace brtc

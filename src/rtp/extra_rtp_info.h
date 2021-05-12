#pragma once
#include <optional>

namespace brtc {

struct ExtraRtpInfo {
    std::optional<bool> is_first_packet_in_frame;
};

} // namespace brtc

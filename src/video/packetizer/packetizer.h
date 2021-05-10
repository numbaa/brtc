
#pragma once
#include <cstdint>
#include <array>
#include <queue>
#include <brtc/frame.h>
#include "common/rtp.h"

namespace brtc
{

class Packetizer {
public:
    struct PayloadSizeLimits {
        int max_payload_len = 1200;
        int first_packet_reduction_len = 0;
        int last_packet_reduction_len = 0;
        // Reduction len for packet that is first & last at the same time.
        int single_packet_reduction_len = 0;
    };

public:
    virtual ~Packetizer() { }
    static std::unique_ptr<Packetizer> create(Frame decoded_frame, VideoCodecType codec_type, PayloadSizeLimits limits);
    bool is_valid_frame() const { return is_valid_frame_; }
    virtual bool next_packet(RtpPacket& packet) = 0;
    virtual bool has_next_packet() const = 0;

protected:
    bool is_valid_frame_ = false;
};

} // namespace brtc


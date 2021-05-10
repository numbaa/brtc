/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#pragma once

#include <array>
#include <brtc/frame.h>
#include <cstdint>
#include <queue>
#include "common/rtp.h"
#include "video/packetizer/packetizer.h"

namespace brtc {

class PacketizerH264 : public Packetizer {

    struct PacketUnit {
        PacketUnit(std::span<uint8_t> source_fragment,
            bool first_fragment,
            bool last_fragment,
            bool aggregated,
            uint8_t header)
            : source_fragment(source_fragment)
            , first_fragment(first_fragment)
            , last_fragment(last_fragment)
            , aggregated(aggregated)
            , header(header)
        {
        }

        std::span<uint8_t> source_fragment;
        bool first_fragment;
        bool last_fragment;
        bool aggregated;
        uint8_t header;
    };

public:
    explicit PacketizerH264(Frame decoded_frame, PayloadSizeLimits limits);
    bool next_packet(RtpPacket& packet) override;
    bool has_next_packet() const override;

private:
    bool do_fragmentation();
    bool do_packetization();
    bool packetize_FuA(size_t index);
    size_t packetize_StapA(size_t index);
    std::vector<int> split_about_equally(int payload_len, const PayloadSizeLimits& limits);
    void next_fragment_packet(RtpPacket& rtp_packet);
    void next_aggregate_packet(RtpPacket& rtp_packet);

private:
    uint32_t current_nalu_ = 0;
    uint32_t nalus_len_ = 0;
    Frame frame_;
    struct Nalu {
        uint32_t start_code_length;
        uint32_t offset;
        uint32_t payload_length;
    };
    //std::array<Nalu, kMaxNalusPerPacket> nalus_;
    std::deque<Nalu> nalus_;
    std::deque<PacketUnit> packets_;
    PayloadSizeLimits limits_;
    size_t num_packets_left_ = 0;
};

} // namespace brtc

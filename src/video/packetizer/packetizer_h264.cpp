/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video/packetizer/packetizer_h264.h"
#include <cassert>

namespace brtc {

namespace {
constexpr size_t kNalHeaderSize = 1;
constexpr size_t kFuAHeaderSize = 2;
constexpr size_t kLengthFieldSize = 2;
// Bit masks for FU (A and B) indicators.
enum NalDefs : uint8_t {
    kFBit = 0x80,
    kNriMask = 0x60,
    kTypeMask = 0x1F
};

// Bit masks for FU (A and B) headers.
enum FuDefs : uint8_t {
    kSBit = 0x80,
    kEBit = 0x40,
    kRBit = 0x20
};

enum NaluType : uint8_t {
    kSlice = 1,
    kIdr = 5,
    kSei = 6,
    kSps = 7,
    kPps = 8,
    kAud = 9,
    kEndOfSequence = 10,
    kEndOfStream = 11,
    kFiller = 12,
    kPrefix = 14,
    kStapA = 24,
    kFuA = 28
};

} // namespace

static std::tuple<uint8_t*, int32_t> find_nalu(std::span<uint8_t> data)
{
    if (data.size() < kH264StartCodeLength) {
        return { nullptr, -1 };
    }
    uint8_t* head = data.data();
    uint8_t* end = data.data() + data.size() - kH264StartCodeLength;
    while (head < end) {
        if (head[0]) {
            head++;
            continue;
        }
        if (head[1]) {
            head += 2;
            continue;
        }
        if (head[2]) {
            if (head[2] == 0x01) {
                return { head, 3 };
            }
        }
        if (head[3] != 0x01) {
            head++;
            continue;
        }
        return { head, 4 };
    }
    return { nullptr, -1 };
}

PacketizerH264::PacketizerH264(Frame encoded_frame, VideoCodecType codec_type, PayloadSizeLimits limits)
    : frame_(encoded_frame)
    , limits_(limits)
{
    switch (codec_type) {
    case brtc::VideoCodecType::H264:
        break;
    case brtc::VideoCodecType::H265:
    case brtc::VideoCodecType::VP8:
    case brtc::VideoCodecType::VP9:
    default:
        assert(false);
    }
    if (!do_fragmentation()) {
        return;
    }
    do_packetization();
}

bool PacketizerH264::next_packet(RtpPacket& rtp_packet)
{
    if (packets_.empty()) {
        return false;
    }
    PacketUnit packet = packets_.front();
    if (packet.first_fragment && packet.last_fragment) {
        // Single NAL unit packet.
        rtp_packet.set_payload(packet.source_fragment);
        packets_.pop_front();
        nalus_.pop_front();
    } else if (packet.aggregated) {
        next_aggregate_packet(rtp_packet);
    } else {
        next_fragment_packet(rtp_packet);
    }
    rtp_packet.set_marker(packets_.empty());
    --num_packets_left_;
    return true;
}

bool PacketizerH264::do_fragmentation()
{
    int32_t index = 0;
    uint8_t* nalus[kMaxNalusPerPacket + 1];
    uint32_t start_code_lens[kMaxNalusPerPacket + 1];
    std::span<uint8_t> remain_data { (uint8_t*)frame_.data, frame_.length };
    while (index < kMaxNalusPerPacket) {
        auto [nalu, start_code_len] = find_nalu(remain_data);
        if (nalu == nullptr) {
            break;
        }
        nalus[index] = nalu;
        start_code_lens[index] = start_code_len;
        index++;
        remain_data = remain_data.subspan(start_code_len);
    }
    if (index == 0) {
        is_valid_frame_ = false;
        return false;
    }
    nalus_len_ = index;
    for (uint32_t i = 0; i < nalus_len_; i++) {
        Nalu nalu;
        nalu.offset = nalus[i] - frame_.data;
        nalu.start_code_length = start_code_lens[i];
        if (i == nalus_len_ - 1) {
            nalu.payload_length = frame_.length - nalu.offset - nalu.start_code_length;
        }
        if (i != 0) {
            nalus_[i - 1].payload_length = nalu.offset - nalus_[i - 1].offset - nalus_[i - 1].start_code_length;
        }
        nalus_[i] = nalu;
    }
    return true;
}

bool PacketizerH264::do_packetization()
{
    constexpr int kMaxPayloadLength = 1200;
    //����stapa �� fu����֧��single
    for (size_t i = 0; i < nalus_.size();) {
        int fragment_len = nalus_[i].payload_length;
        int single_packet_capacity = kMaxPayloadLength;
        if (nalus_.size() == 1)
            single_packet_capacity -= limits_.single_packet_reduction_len;
        else if (i == 0)
            single_packet_capacity -= limits_.first_packet_reduction_len;
        else if (i + 1 == nalus_.size())
            single_packet_capacity -= limits_.last_packet_reduction_len;

        if (fragment_len > single_packet_capacity) {
            if (!packetize_FuA(i))
                return false;
            ++i;
        } else {
            i = packetize_StapA(i);
        }
    }
    return true;
}

// copy from webrtc
bool PacketizerH264::packetize_FuA(size_t index)
{
    // Fragment payload into packets (FU-A).
    std::span<uint8_t> fragment { (uint8_t*)frame_.data + nalus_[index].offset + nalus_[index].start_code_length, nalus_[index].payload_length };

    PayloadSizeLimits limits = limits_;
    // Leave room for the FU-A header.
    limits.max_payload_len -= kFuAHeaderSize;
    // Update single/first/last packet reductions unless it is single/first/last
    // fragment.
    if (nalus_.size() != 1) {
        // if this fragment is put into a single packet, it might still be the
        // first or the last packet in the whole sequence of packets.
        if (index == nalus_.size() - 1) {
            limits.single_packet_reduction_len = limits_.last_packet_reduction_len;
        } else if (index == 0) {
            limits.single_packet_reduction_len = limits_.first_packet_reduction_len;
        } else {
            limits.single_packet_reduction_len = 0;
        }
    }
    if (index != 0)
        limits.first_packet_reduction_len = 0;
    if (index != nalus_.size() - 1)
        limits.last_packet_reduction_len = 0;

    // Strip out the original header.
    size_t payload_left = fragment.size() - kNalHeaderSize;
    int offset = kNalHeaderSize;

    std::vector<int> payload_sizes = split_about_equally(payload_left, limits);
    if (payload_sizes.empty())
        return false;

    for (size_t i = 0; i < payload_sizes.size(); ++i) {
        int packet_length = payload_sizes[i];
        assert(packet_length == 0);
        packets_.push_back(PacketUnit(fragment.subspan(offset, packet_length),
            /*first_fragment=*/i == 0,
            /*last_fragment=*/i == payload_sizes.size() - 1,
            false, fragment[0]));
        offset += packet_length;
        payload_left -= packet_length;
    }
    num_packets_left_ += payload_sizes.size();
    assert(0 == payload_left);
    return true;
}

//copy from webrtc
size_t PacketizerH264::packetize_StapA(size_t index)
{
    // Aggregate fragments into one packet (STAP-A).
    size_t payload_size_left = limits_.max_payload_len;
    if (nalus_.size() == 1)
        payload_size_left -= limits_.single_packet_reduction_len;
    else if (index == 0)
        payload_size_left -= limits_.first_packet_reduction_len;
    int aggregated_fragments = 0;
    size_t fragment_headers_length = 0;
    std::span<uint8_t> fragment { (uint8_t*)frame_.data + nalus_[index].offset + nalus_[index].start_code_length, nalus_[index].payload_length };
    assert(payload_size_left > fragment.size());
    ++num_packets_left_;

    auto payload_size_needed = [&] {
        size_t fragment_size = fragment.size() + fragment_headers_length;
        if (nalus_.size() == 1) {
            // Single fragment, single packet, payload_size_left already adjusted
            // with limits_.single_packet_reduction_len.
            return fragment_size;
        }
        if (index == nalus_.size() - 1) {
            // Last fragment, so STAP-A might be the last packet.
            return fragment_size + limits_.last_packet_reduction_len;
        }
        return fragment_size;
    };

    while (payload_size_left >= payload_size_needed()) {
        assert(fragment.size() > 0);
        packets_.push_back(PacketUnit(fragment, aggregated_fragments == 0, false, true,
            fragment[0]));
        payload_size_left -= fragment.size();
        payload_size_left -= fragment_headers_length;

        fragment_headers_length = kLengthFieldSize;
        // If we are going to try to aggregate more fragments into this packet
        // we need to add the STAP-A NALU header and a length field for the first
        // NALU of this packet.
        if (aggregated_fragments == 0)
            fragment_headers_length += kNalHeaderSize + kLengthFieldSize;
        ++aggregated_fragments;

        // Next fragment.
        ++index;
        if (index == nalus_.size())
            break;
        std::span<uint8_t> fragment { (uint8_t*)frame_.data + nalus_[index].offset + nalus_[index].start_code_length, nalus_[index].payload_length };
    }
    assert(aggregated_fragments > 0);
    packets_.back().last_fragment = true;
    return index;
}

std::vector<int> PacketizerH264::split_about_equally(int payload_len, const PayloadSizeLimits& limits)
{
    assert(payload_len > 0);
    // First or last packet larger than normal are unsupported.
    assert(limits.first_packet_reduction_len >= 0);
    assert(limits.last_packet_reduction_len >= 0);

    std::vector<int> result;
    if (limits.max_payload_len >= limits.single_packet_reduction_len + payload_len) {
        result.push_back(payload_len);
        return result;
    }
    if (limits.max_payload_len - limits.first_packet_reduction_len < 1 || limits.max_payload_len - limits.last_packet_reduction_len < 1) {
        // Capacity is not enough to put a single byte into one of the packets.
        return result;
    }
    // First and last packet of the frame can be smaller. Pretend that it's
    // the same size, but we must write more payload to it.
    // Assume frame fits in single packet if packet has extra space for sum
    // of first and last packets reductions.
    int total_bytes = payload_len + limits.first_packet_reduction_len + limits.last_packet_reduction_len;
    // Integer divisions with rounding up.
    int num_packets_left = (total_bytes + limits.max_payload_len - 1) / limits.max_payload_len;
    if (num_packets_left == 1) {
        // Single packet is a special case handled above.
        num_packets_left = 2;
    }

    if (payload_len < num_packets_left) {
        // Edge case where limits force to have more packets than there are payload
        // bytes. This may happen when there is single byte of payload that can't be
        // put into single packet if
        // first_packet_reduction + last_packet_reduction >= max_payload_len.
        return result;
    }

    int bytes_per_packet = total_bytes / num_packets_left;
    int num_larger_packets = total_bytes % num_packets_left;
    int remaining_data = payload_len;

    result.reserve(num_packets_left);
    bool first_packet = true;
    while (remaining_data > 0) {
        // Last num_larger_packets are 1 byte wider than the rest. Increase
        // per-packet payload size when needed.
        if (num_packets_left == num_larger_packets)
            ++bytes_per_packet;
        int current_packet_bytes = bytes_per_packet;
        if (first_packet) {
            if (current_packet_bytes > limits.first_packet_reduction_len + 1)
                current_packet_bytes -= limits.first_packet_reduction_len;
            else
                current_packet_bytes = 1;
        }
        if (current_packet_bytes > remaining_data) {
            current_packet_bytes = remaining_data;
        }
        // This is not the last packet in the whole payload, but there's no data
        // left for the last packet. Leave at least one byte for the last packet.
        if (num_packets_left == 2 && current_packet_bytes == remaining_data) {
            --current_packet_bytes;
        }
        result.push_back(current_packet_bytes);

        remaining_data -= current_packet_bytes;
        --num_packets_left;
        first_packet = false;
    }

    return result;
}

/*
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | FU indicator  |   FU header   |                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
    |                                                               |
    |                         FU payload                            |
    |                                                               |
    |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                               :...OPTIONAL RTP padding        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
void PacketizerH264::next_fragment_packet(RtpPacket& rtp_packet)
{
    PacketUnit* packet = &packets_.front();
    // NAL unit fragmented over multiple packets (FU-A).
    // We do not send original NALU header, so it will be replaced by the
    // FU indicator header of the first packet.
    uint8_t fu_indicator = (packet->header & (kFBit | kNriMask)) | NaluType::kFuA;
    uint8_t fu_header = 0;

    // S | E | R | 5 bit type.
    fu_header |= (packet->first_fragment ? kSBit : 0);
    fu_header |= (packet->last_fragment ? kEBit : 0);
    uint8_t type = packet->header & kTypeMask;
    fu_header |= type;
    auto fragment = packet->source_fragment;
    std::vector<uint8_t> buffer(kFuAHeaderSize + fragment.size());
    buffer[0] = fu_indicator;
    buffer[1] = fu_header;
    memcpy(buffer.data() + kFuAHeaderSize, fragment.data(), fragment.size());
    rtp_packet.set_payload(std::move(buffer));
    if (packet->last_fragment)
        nalus_.pop_front();
    packets_.pop_front();
}

/*
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          RTP Header                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                         NALU 1 Data                           |
    :                                                               :
    +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |               | NALU 2 Size                   | NALU 2 HDR    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                         NALU 2 Data                           |
    :                                                               :
    |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                               :...OPTIONAL RTP padding        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
void PacketizerH264::next_aggregate_packet(RtpPacket& rtp_packet)
{
    // Reserve maximum available payload, set actual payload size later.
    size_t payload_len = kNalHeaderSize;
    for (auto& packet : packets_) {
        payload_len += kLengthFieldSize + kNalHeaderSize + packet.source_fragment.size();
    }
    std::vector<uint8_t> buffer(payload_len);
    PacketUnit* packet = &packets_.front();
    assert(packet->first_fragment == true);
    // STAP-A NALU header.
    buffer[0] = (packet->header & (kFBit | kNriMask)) | NaluType::kStapA;
    size_t index = kNalHeaderSize;
    bool is_last_fragment = packet->last_fragment;
    while (packet->aggregated) {
        auto fragment = packet->source_fragment;
        // Add NAL unit length field.
        uint16_t fragment_len = static_cast<uint16_t>(fragment.size());
        //write big endian
        buffer[index] = fragment_len >> 8;
        buffer[index + 1] = static_cast<uint16_t>(fragment_len & 0x00ff);
        index += kLengthFieldSize;
        // Add NAL unit.
        memcpy(&buffer[index], fragment.data(), fragment.size());
        index += fragment.size();
        packets_.pop_front();
        nalus_.pop_front();
        if (is_last_fragment)
            break;
        packet = &packets_.front();
        is_last_fragment = packet->last_fragment;
    }
    assert(is_last_fragment);
    rtp_packet.set_payload(std::move(buffer));
}

} // namespace brtc

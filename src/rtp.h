#pragma once

#include <cstdint>
#include <vector>

namespace brtc
{

constexpr uint32_t kMaxNalusPerPacket = 10;
constexpr uint32_t kH264StartCodeLength = 4;

enum class H264PacketizationTypes {
    kH264SingleNalu,
    kH264StapA,
    kH264FuA,
};

enum class H264PacketizationMode {
    SingleNalUnit,
    NonInterleaved,
};

struct NaluInfo {
    uint8_t type;
    int sps_id;
    int pps_id;
};

//编码后输出的Frame的header
struct RTPVideoHeaderH264 {
    uint8_t nalu_type;
    H264PacketizationTypes packetization_type;
    NaluInfo nalus[kMaxNalusPerPacket];
    uint32_t nalus_length;
    H264PacketizationMode packetization_mode;
    uint16_t picture_id;
    int64_t timestamp_local;
    bool has_last_fragement;
};

//class RtpPacketToSend继承class RtpPacket
struct RtpPacket {
    bool marker;
    uint8_t  payload_type;
    uint8_t padding_size;
    uint16_t sequence_number;
    uint32_t ssrc;
    uint32_t payload_offset;
    uint32_t payload_size;
    std::span<uint8_t> frame_;
    uint32_t offset_of_frame_;
};

} // namespace brtc

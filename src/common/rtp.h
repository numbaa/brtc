#pragma once

#include <cstdint>
#include <vector>
#include <span>

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

class RtpPacket {
public:
    RtpPacket();

    bool marker() const;
    uint8_t payload_type() const;
    uint16_t sequence_number() const;
    uint32_t timestamp() const;
    uint32_t ssrc() const;
    std::vector<uint32_t> csrcs() const;
    size_t csrcs_size() const;
    size_t headers_size() const;
    size_t payload_size() const;
    size_t padding_size() const;
    size_t extensions_size() const;
    std::span<uint8_t> payload() const;
    size_t size() const;
    const uint8_t* data() const;

    void set_marker(bool marker);
    void set_payload_type(uint8_t pt);
    void set_sequence_number(uint16_t seq);
    void set_timestamp(uint32_t timestamp);
    void set_ssrc(uint32_t ssrc);
    void set_csrcs(std::span<uint32_t> csrcs);

private:
    size_t extensions_size_;
    std::vector<uint8_t> buffer_;
};

} // namespace brtc

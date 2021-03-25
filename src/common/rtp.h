#pragma once

#include <cassert>
#include <cstdint>
#include <vector>
#include <span>
#include <concepts>

#include <bco/buffer.h>

#include <brtc/frame.h>

namespace brtc
{

constexpr uint32_t kMaxNalusPerPacket = 10;
constexpr uint32_t kH264StartCodeLength = 4;
constexpr uint32_t kOneByteHeaderExtensionMaxId = 14;
constexpr uint32_t kOneByteHeaderExtensionMaxValueSize = 16;

enum class RTPExtensionType : int {
    kRtpExtensionNone,
    kRtpExtensionTransmissionTimeOffset,
    kRtpExtensionAudioLevel,
    kRtpExtensionInbandComfortNoise,
    kRtpExtensionAbsoluteSendTime,
    kRtpExtensionAbsoluteCaptureTime,
    kRtpExtensionVideoRotation,
    kRtpExtensionTransportSequenceNumber,
    kRtpExtensionTransportSequenceNumber02,
    kRtpExtensionPlayoutDelay,
    kRtpExtensionVideoContentType,
    kRtpExtensionVideoLayersAllocation,
    kRtpExtensionVideoTiming,
    kRtpExtensionRtpStreamId,
    kRtpExtensionRepairedRtpStreamId,
    kRtpExtensionMid,
    kRtpExtensionGenericFrameDescriptor00,
    kRtpExtensionGenericFrameDescriptor = kRtpExtensionGenericFrameDescriptor00,
    kRtpExtensionGenericFrameDescriptor02,
    kRtpExtensionColorSpace,
    kRtpExtensionNumberOfExtensions // Must be the last entity in the enum.
};

template <typename T>
concept RtpExtension = requires(bco::Buffer buff, typename T::value_type& ref_value, const typename T::value_type& const_value)
{
    {
        T::kId
    }
    ->std::same_as<const RTPExtensionType>;
    {
        T::kValueSizeBytes
    }
    ->std::same_as<const uint8_t>;
    {
        T::kUri
    }
    ->std::same_as<const char*>;
    {
        T::read_from_buff(buff, ref_value)
    }
    ->std::same_as<bool>;
    {
        T::write_to_buff(buff, const_value)
    }
    ->std::same_as<bool>;
};

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
    RtpPacket(bco::Buffer buff);

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
    template <typename T> requires RtpExtension<T>
    bool get_extension(typename T::value_type& ext) const;
    const bco::Buffer payload() const;
    size_t size() const;
    const bco::Buffer data() const;

    void set_marker(bool marker);
    void set_payload_type(uint8_t pt);
    void set_sequence_number(uint16_t seq);
    void set_timestamp(uint32_t timestamp);
    void set_ssrc(uint32_t ssrc);
    void set_csrcs(std::span<uint32_t> csrcs);
    template <typename T> requires RtpExtension<T>
    bool set_extension(const typename T::value_type& ext);
    void set_payload(const std::span<uint8_t>& payload);
    void set_payload(std::vector<uint8_t>&& payload);

    void set_frame(Frame frame);

private:
    void parse();
    bco::Buffer find_extension(RTPExtensionType type);
    
    template <typename T> requires RtpExtension<T>
    bool need_promotion() const;

    template <typename T> requires RtpExtension<T>
    bool need_more_buffer_space() const;

    template <typename T> requires RtpExtension<T>
    bool push_back_extension(const typename T::value_type& value);

    bool promote_two_bytes_header_and_reserve_n_bytes(uint8_t n_bytes);

    bool allocate_n_bytes(uint8_t bytes);

private:
    struct ExtensionInfo {
        explicit ExtensionInfo(RTPExtensionType _type)
            : ExtensionInfo(_type, 0, 0)
        {
        }
        ExtensionInfo(RTPExtensionType _type, uint8_t _offset, uint16_t _length)
            : type(_type)
            , offset(_offset)
            , length(_length)
        {
        }
        RTPExtensionType type;
        uint16_t offset;
        uint8_t length;
    };
    enum class ExtensionMode {
        kOneByte,
        kTwoByte,
    };

private:
    ExtensionMode extension_mode_ = ExtensionMode::kOneByte;
    size_t extensions_size_;
    std::vector<ExtensionInfo> extension_entries_;
    mutable bco::Buffer buffer_;
    mutable Frame frame_;
};



template <typename T> requires RtpExtension<T>
inline bool RtpPacket::get_extension(typename T::value_type& value) const
{
    auto buff = find_extension(T::kId);
    if (buff.empty()) {
        return false;
    }
    return T::read_from_buff(buff, value);
}

template <typename T> requires RtpExtension<T>
inline bool RtpPacket::set_extension(const typename T::value_type& value)
{
    auto buff = find_extension(T::kId);
    if (!buff.empty()) {
        return T::write_to_buff(buff, value);
    }
    if (need_promotion<T>()) {
        promote_two_bytes_header_and_reserve_n_bytes(T::kValueSizeBytes + 2);
    } else { //if (need_more_buffer_space<T>()) { 空间不是预留式的，所以不需要need_more_buffer_space
        size_t size = extension_mode_ == ExtensionMode::kOneByte ? T::kValueSizeBytes + 1 : T::kValueSizeBytes + 2;
        allocate_n_bytes(size);
    }
    return push_back_extension<T>(value);
}

template <typename T> requires RtpExtension<T>
inline bool RtpPacket::need_promotion() const
{
    assert(T::kId != 15 && T::kId != 0);
    return extension_mode_ == ExtensionMode::kOneByte
        && (T::kId > kOneByteHeaderExtensionMaxId || T::kValueSizeBytes > kOneByteHeaderExtensionMaxValueSize);
}

template <typename T>  requires RtpExtension<T>
inline bool RtpPacket::need_more_buffer_space() const
{
    //extension element之间似乎还有padding，比较麻烦算
    return false;
}

template <typename T>
inline bool RtpPacket::push_back_extension(const typename T::value_type& value)
{
    size_t insert_pos;
    if (not extension_entries_.empty()) {
        insert_pos = extension_entries_.back().offset + extension_entries_.back().length;
    } else {
        insert_pos = kFixedHeaderSize + csrcs_size() * sizeof(uint32_t) + sizeof(uint32_t);
    }
    if (extension_mode_ == ExtensionMode::kOneByte) {
        buffer_[insert_pos] = (T::kId << 4) | (T::kValueSizeBytes - 1);
        T::write_to_buff(buffer_.subbuf(insert_pos + 1, T::kValueSizeBytes), value);
        extension_entries_.push_back(ExtensionInfo {T::kId, insert_pos, T::kValueSizeBytes + 1});
    } else {
        buffer_[insert_pos] = T::kId;
        buffer_[insert_pos + 1] = T::kValueSizeBytes - 1;
        T::write_to_buff(buffer_.subbuf(insert_pos + 2, T::kValueSizeBytes), value);
        extension_entries_.push_back(ExtensionInfo { T::kId, insert_pos, T::kValueSizeBytes + 2 });
    }
    return true;
}

} // namespace brtc

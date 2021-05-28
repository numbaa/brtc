#pragma once

#include <cassert>
#include <cstdint>
#include <vector>
#include <span>
#include <concepts>
#include <variant>
#include <bitset>
#include <optional>

#include <bco/buffer.h>

#include <brtc/frame.h>
#include <brtc/video/codec_type.h>
#include <brtc/video/dependency_descriptor.h>
#include <brtc/video/color_space.h>
#include <brtc/video/codec_interface.h>
#include <brtc/video/vp9_globals.h>
#include <brtc/video/vp8_globals.h>
#include "rtp/extension.h"
#include "rtp/extra_rtp_info.h"


namespace brtc
{

constexpr uint32_t kMaxNalusPerPacket = 10;
constexpr uint32_t kH264StartCodeLength = 4;
constexpr uint32_t kOneByteHeaderExtensionMaxId = 14;
constexpr uint32_t kOneByteHeaderExtensionMaxValueSize = 16;

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

enum H264NaluType : uint8_t {
    Slice = 1,
    Idr = 5,
    Sei = 6,
    Sps = 7,
    Pps = 8,
    Aud = 9,
    EndOfSequence = 10,
    EndOfStream = 11,
    Filler = 12,
    StapA = 24,
    FuA = 28
};

// RTCP mode to use. Compound mode is described by RFC 4585 and reduced-size
// RTCP mode is described by RFC 5506.
enum class RtcpMode { kOff,
    kCompound,
    kReducedSize
};

//// Relationship of a frame to a Decode target.
//enum class DecodeTargetIndication {
//    kNotPresent = 0, // DecodeTargetInfo symbol '-'
//    kDiscardable = 1, // DecodeTargetInfo symbol 'D'
//    kSwitch = 2, // DecodeTargetInfo symbol 'S'
//    kRequired = 3 // DecodeTargetInfo symbol 'R'
//};

enum class VideoRotation {
    kVideoRotation_0 = 0,
    kVideoRotation_90 = 90,
    kVideoRotation_180 = 180,
    kVideoRotation_270 = 270
};

enum class VideoContentType : uint8_t {
    UNSPECIFIED = 0,
    SCREENSHARE = 1,
};

// Minimum and maximum playout delay values from capture to render.
// These are best effort values.
//
// A value < 0 indicates no change from previous valid value.
//
// min = max = 0 indicates that the receiver should try and render
// frame as soon as possible.
//
// min = x, max = y indicates that the receiver is free to adapt
// in the range (x, y) based on network jitter.
struct VideoPlayoutDelay {
    VideoPlayoutDelay() = default;
    VideoPlayoutDelay(int min_ms, int max_ms)
        : min_ms(min_ms)
        , max_ms(max_ms)
    {
    }
    int min_ms = -1;
    int max_ms = -1;

    bool operator==(const VideoPlayoutDelay& rhs) const
    {
        return min_ms == rhs.min_ms && max_ms == rhs.max_ms;
    }
};

struct RTPVideoHeader {
    struct GenericDescriptorInfo {
        GenericDescriptorInfo() = default;
        GenericDescriptorInfo(const GenericDescriptorInfo& other) = default;
        ~GenericDescriptorInfo() = default;

        int64_t frame_id = 0;
        int spatial_index = 0;
        int temporal_index = 0;
        std::vector<webrtc::DecodeTargetIndication> decode_target_indications;
        std::vector<int64_t> dependencies;
        std::vector<int> chain_diffs;
        std::bitset<32> active_decode_targets = ~uint32_t { 0 };
    };
    std::optional<GenericDescriptorInfo> generic;
    VideoFrameType frame_type = VideoFrameType::EmptyFrame;
    uint16_t width = 0;
    uint16_t height = 0;
    //VideoRotation rotation = VideoRotation::kVideoRotation_0; // useless
    //VideoContentType content_type = VideoContentType::UNSPECIFIED; //useless
    bool is_first_packet_in_frame = false;
    bool is_last_packet_in_frame = false;
    bool is_last_frame_in_picture = true;
    uint8_t simulcastIdx = 0;
    VideoCodecType codec = VideoCodecType::Generic;

    //VideoPlayoutDelay playout_delay; //useless
    //VideoSendTiming video_timing; //useless
    std::optional<webrtc::ColorSpace> color_space;
    // This field is meant for media quality testing purpose only. When enabled it
    // carries the webrtc::VideoFrame id field from the sender to the receiver.
    //std::optional<uint16_t> video_frame_tracking_id;
};

struct RTPVideoHeaderH264 : public RTPVideoHeader {
    H264NaluType nalu_type;
    H264PacketizationTypes packetization_type;
    NaluInfo nalus[kMaxNalusPerPacket];
    uint32_t nalus_length;
    H264PacketizationMode packetization_mode;
    uint16_t picture_id;
    int64_t timestamp_local;
    bool has_last_fragement;
};

struct RTPVideoHeaderH265 : public RTPVideoHeader {
};

struct RTPVideoHeaderVP8 : public RTPVideoHeader, public webrtc::RTPVideoHeaderVP8 {
    /*
    void InitRTPVideoHeaderVP8()
    {
        nonReference = false;
        pictureId = kNoPictureId;
        tl0PicIdx = kNoTl0PicIdx;
        temporalIdx = kNoTemporalIdx;
        layerSync = false;
        keyIdx = kNoKeyIdx;
        partitionId = 0;
        beginningOfPartition = false;
    }

    bool nonReference; // Frame is discardable.
    int16_t pictureId; // Picture ID index, 15 bits;
        // kNoPictureId if PictureID does not exist.
    int16_t tl0PicIdx; // TL0PIC_IDX, 8 bits;
        // kNoTl0PicIdx means no value provided.
    uint8_t temporalIdx; // Temporal layer index, or kNoTemporalIdx.
    bool layerSync; // This frame is a layer sync frame.
        // Disabled if temporalIdx == kNoTemporalIdx.
    int keyIdx; // 5 bits; kNoKeyIdx means not used.
    int partitionId; // VP8 partition ID
    bool beginningOfPartition; // True if this packet is the first
        // in a VP8 partition. Otherwise false
    */
};

struct RTPVideoHeaderVP9 : public RTPVideoHeader, public webrtc::RTPVideoHeaderVP9 {
};

using UnionRTPVideoHeader = std::variant<RTPVideoHeader, RTPVideoHeaderH264, RTPVideoHeaderH265, RTPVideoHeaderVP8, RTPVideoHeaderVP9>;

constexpr size_t kMaxFrameReferences = 5;
struct ReceivedFrame : public Frame {
    VideoCodecType codec_type;
    VideoFrameType frame_type;
    UnionRTPVideoHeader video_header;
    int spatial_index;
    uint16_t first_seq_num;
    uint16_t last_seq_num;
    int64_t id;
    size_t num_references;
    int64_t references[kMaxFrameReferences];
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
    bool empty_payload() const;
    const bco::Buffer data() const;
    template <typename T>
    const T& video_header() const {
        return std::get<T>(video_header_);
    }
    template <typename T>
    T& video_header() {
        return std::get<T>(video_header_);
    }
    //const ExtraRtpInfo& extra_info() const;
    //ExtraRtpInfo& extra_info();

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
    template <typename T>
    void set_video_header(const T& header) {
        video_header_ = header;
    }

private:
    void parse();
    bco::Buffer find_extension(RTPExtensionType type) const;
    
    template <typename T> requires RtpExtension<T>
    bool need_promotion(const typename T::value_type& value) const;

    template <typename T> requires RtpExtension<T>
    bool need_more_buffer_space() const;

    template <typename T> requires RtpExtension<T>
    bool push_back_extension(const typename T::value_type& value);

    void promote_two_bytes_header_and_reserve_n_bytes(uint8_t n_bytes);

    void allocate_n_bytes_for_extension(uint8_t bytes);

private:
    struct ExtensionInfo {
        explicit ExtensionInfo(RTPExtensionType _type)
            : ExtensionInfo(_type, 0, 0)
        {
        }
        ExtensionInfo(RTPExtensionType _type, uint8_t _offset, uint8_t _length)
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

    ExtensionInfo& find_or_create_extension_info(RTPExtensionType type);

private:
    ExtensionMode extension_mode_ = ExtensionMode::kOneByte;
    std::vector<ExtensionInfo> extension_entries_;
    std::variant<RTPVideoHeader, RTPVideoHeaderH264, RTPVideoHeaderH265, RTPVideoHeaderVP8, RTPVideoHeaderVP9> video_header_;
    //ExtraRtpInfo extra_rtp_info_;
    mutable bco::Buffer buffer_;
    //mutable Frame frame_;
};



template <typename T> requires RtpExtension<T>
inline bool RtpPacket::get_extension(typename T::value_type& value) const
{
    auto buff = find_extension(T::id());
    if (buff.size() == 0) {
        return false;
    }
    return T::read_from_buff(buff, value);
}

template <typename T> requires RtpExtension<T>
inline bool RtpPacket::set_extension(const typename T::value_type& value)
{
    buffer_[0] |= 0b0001'0000;
    auto buff = find_extension(T::id());
    if (buff.size() != 0) {
        return T::write_to_buff(buff, value);
    }
    if (need_promotion<T>(value)) {
        promote_two_bytes_header_and_reserve_n_bytes(T::value_size(value) + 2);
    } else { //if (need_more_buffer_space<T>()) { 空间不是预留式的，所以不需要need_more_buffer_space
        size_t size = extension_mode_ == ExtensionMode::kOneByte ? T::value_size(value) + 1 : T::value_size(value) + 2;
        allocate_n_bytes_for_extension(size);
    }
    return push_back_extension<T>(value);
}

template <typename T> requires RtpExtension<T>
inline bool RtpPacket::need_promotion(const typename T::value_type& value) const
{
    uint32_t id = static_cast<uint32_t>(T::id());
    assert(id != 15 && id != 0);
    return extension_mode_ == ExtensionMode::kOneByte
        && (id > kOneByteHeaderExtensionMaxId || T::value_size(value) > kOneByteHeaderExtensionMaxValueSize);
}

template <typename T>  requires RtpExtension<T>
inline bool RtpPacket::need_more_buffer_space() const
{
    //extension element之间似乎还有padding，比较麻烦算
    return false;
}

template <typename T> requires RtpExtension<T>
bool RtpPacket::push_back_extension(const typename T::value_type& value)
{
    constexpr size_t kFixedHeaderSize = 12;
    uint8_t insert_pos;
    if (not extension_entries_.empty()) {
        insert_pos = extension_entries_.back().offset + extension_entries_.back().length;
    } else {
        insert_pos = kFixedHeaderSize + csrcs_size() * sizeof(uint32_t) + sizeof(uint32_t);
    }
    const uint8_t id = static_cast<uint8_t>(T::id());
    const uint8_t value_size = T::value_size(value);
    if (extension_mode_ == ExtensionMode::kOneByte) {
        buffer_[insert_pos] = (id << 4) | (value_size - 1);
        T::write_to_buff(buffer_.subbuf(insert_pos + 1, value_size), value);
        extension_entries_.push_back(ExtensionInfo { T::id(), insert_pos, uint8_t ( value_size + 1 ) });
    } else {
        buffer_[insert_pos] = id;
        buffer_[insert_pos + 1] = value_size;
        T::write_to_buff(buffer_.subbuf(insert_pos + 2, value_size), value);
        extension_entries_.push_back(ExtensionInfo { T::id(), insert_pos, uint8_t ( value_size + 2 ) });
    }
    return true;
}

class RtcpPacket {
};

} // namespace brtc

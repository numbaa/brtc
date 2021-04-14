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

#include "video/reference_finder/vp9_globals.h"
#include "video/reference_finder/vp8_globals.h"

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

enum class VideoCodecType : uint8_t {
    Unknown,
    H264,
    H265,
    VP8,
    VP9,
};

enum class VideoFrameType {
    EmptyFrame = 0,
    VideoFrameKey = 3,
    VideoFrameDelta = 4,
};

// Relationship of a frame to a Decode target.
enum class DecodeTargetIndication {
    kNotPresent = 0, // DecodeTargetInfo symbol '-'
    kDiscardable = 1, // DecodeTargetInfo symbol 'D'
    kSwitch = 2, // DecodeTargetInfo symbol 'S'
    kRequired = 3 // DecodeTargetInfo symbol 'R'
};

struct RTPVideoHeader {
    struct GenericDescriptorInfo {
        GenericDescriptorInfo() = default;
        GenericDescriptorInfo(const GenericDescriptorInfo& other) = default;
        ~GenericDescriptorInfo() = default;

        int64_t frame_id = 0;
        int spatial_index = 0;
        int temporal_index = 0;
        std::vector<DecodeTargetIndication> decode_target_indications;
        std::vector<int64_t> dependencies;
        std::vector<int> chain_diffs;
        std::bitset<32> active_decode_targets = ~uint32_t { 0 };
    };
    std::optional<GenericDescriptorInfo> generic;
    VideoFrameType frame_type = VideoFrameType::EmptyFrame;
    uint16_t width = 0;
    uint16_t height = 0;
    bool is_first_packet_in_frame = false;
    bool is_last_packet_in_frame = false;
    bool is_last_frame_in_picture = true;
    uint8_t simulcastIdx = 0;
    VideoCodecType codec = VideoCodecType::Unknown;
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
    /*
    void InitRTPVideoHeaderVP9()
    {
        inter_pic_predicted = false;
        flexible_mode = false;
        beginning_of_frame = false;
        end_of_frame = false;
        ss_data_available = false;
        non_ref_for_inter_layer_pred = false;
        picture_id = kNoPictureId;
        max_picture_id = webrtc::kMaxTwoBytePictureId;
        tl0_pic_idx = kNoTl0PicIdx;
        temporal_idx = kNoTemporalIdx;
        spatial_idx = webrtc::kNoSpatialIdx;
        temporal_up_switch = false;
        inter_layer_predicted = false;
        gof_idx = webrtc::kNoGofIdx;
        num_ref_pics = 0;
        num_spatial_layers = 1;
        first_active_layer = 0;
        end_of_picture = true;
    }

    bool inter_pic_predicted; // This layer frame is dependent on previously
        // coded frame(s).
    bool flexible_mode; // This frame is in flexible mode.
    bool beginning_of_frame; // True if this packet is the first in a VP9 layer
        // frame.
    bool end_of_frame; // True if this packet is the last in a VP9 layer frame.
    bool ss_data_available; // True if SS data is available in this payload
        // descriptor.
    bool non_ref_for_inter_layer_pred; // True for frame which is not used as
        // reference for inter-layer prediction.
    int16_t picture_id; // PictureID index, 15 bits;
        // kNoPictureId if PictureID does not exist.
    int16_t max_picture_id; // Maximum picture ID index; either 0x7F or 0x7FFF;
    int16_t tl0_pic_idx; // TL0PIC_IDX, 8 bits;
        // kNoTl0PicIdx means no value provided.
    uint8_t temporal_idx; // Temporal layer index, or kNoTemporalIdx.
    uint8_t spatial_idx; // Spatial layer index, or kNoSpatialIdx.
    bool temporal_up_switch; // True if upswitch to higher frame rate is possible
        // starting from this frame.
    bool inter_layer_predicted; // Frame is dependent on directly lower spatial
        // layer frame.

    uint8_t gof_idx; // Index to predefined temporal frame info in SS data.

    uint8_t num_ref_pics; // Number of reference pictures used by this layer
        // frame.
    uint8_t pid_diff[webrtc::kMaxVp9RefPics]; // P_DIFF signaled to derive the PictureID
        // of the reference pictures.
    int16_t ref_picture_id[webrtc::kMaxVp9RefPics]; // PictureID of reference pictures.

    // SS data.
    size_t num_spatial_layers; // Always populated.
    size_t first_active_layer; // Not sent on wire, used to adjust ss data.
    bool spatial_layer_resolution_present;
    uint16_t width[webrtc::kMaxVp9NumberOfSpatialLayers];
    uint16_t height[webrtc::kMaxVp9NumberOfSpatialLayers];
    webrtc::GofInfoVP9 gof;

    bool end_of_picture; // This frame is the last frame in picture.
    */
};

constexpr size_t kMaxFrameReferences = 5;
struct ReceivedFrame : public Frame {
    VideoCodecType codec_type;
    VideoFrameType frame_type;
    std::variant<RTPVideoHeader, RTPVideoHeaderH264, RTPVideoHeaderH265, RTPVideoHeaderVP8, RTPVideoHeaderVP9> video_header;
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
    const bco::Buffer data() const;
    template <typename T>
    const T& video_header() const {
        return std::get<T>(video_header_);
    }
    template <typename T>
    T& video_header() {
        return std::get<T>(video_header_);
    }

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
    bco::Buffer find_extension(RTPExtensionType type);
    
    template <typename T> requires RtpExtension<T>
    bool need_promotion() const;

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
    std::vector<ExtensionInfo> extension_entries_;
    std::variant<RTPVideoHeader, RTPVideoHeaderH264, RTPVideoHeaderH265, RTPVideoHeaderVP8, RTPVideoHeaderVP9> video_header_;
    mutable bco::Buffer buffer_;
    //mutable Frame frame_;
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
        allocate_n_bytes_for_extension(size);
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

class RtcpPacket {
};

} // namespace brtc

#pragma once
#include <cstdint>
#include <concepts>
#include <bco/buffer.h>

namespace brtc {

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
        T::id()
    }
    ->std::same_as<RTPExtensionType>;
    {
        T::value_size(const_value)
    }
    ->std::same_as<uint8_t>;
    {
        T::uri()
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

// Data to put on the wire for FrameDescriptor rtp header extension.
class RtpGenericFrameDescriptor {
public:
    static constexpr int kMaxNumFrameDependencies = 8;
    static constexpr int kMaxTemporalLayers = 8;
    static constexpr int kMaxSpatialLayers = 8;

    RtpGenericFrameDescriptor();
    RtpGenericFrameDescriptor(const RtpGenericFrameDescriptor&);
    ~RtpGenericFrameDescriptor();

    bool FirstPacketInSubFrame() const { return beginning_of_subframe_; }
    void SetFirstPacketInSubFrame(bool first) { beginning_of_subframe_ = first; }
    bool LastPacketInSubFrame() const { return end_of_subframe_; }
    void SetLastPacketInSubFrame(bool last) { end_of_subframe_ = last; }

    // Properties below undefined if !FirstPacketInSubFrame()
    // Valid range for temporal layer: [0, 7]
    int TemporalLayer() const;
    void SetTemporalLayer(int temporal_layer);

    // Frame might by used, possible indirectly, for spatial layer sid iff
    // (bitmask & (1 << sid)) != 0
    int SpatialLayer() const;
    uint8_t SpatialLayersBitmask() const;
    void SetSpatialLayersBitmask(uint8_t spatial_layers);

    int Width() const { return width_; }
    int Height() const { return height_; }
    void SetResolution(int width, int height);

    uint16_t FrameId() const;
    void SetFrameId(uint16_t frame_id);

    std::span<const uint16_t> FrameDependenciesDiffs() const;
    void ClearFrameDependencies() { num_frame_deps_ = 0; }
    // Returns false on failure, i.e. number of dependencies is too large.
    bool AddFrameDependencyDiff(uint16_t fdiff);

private:
    bool beginning_of_subframe_ = false;
    bool end_of_subframe_ = false;

    uint16_t frame_id_ = 0;
    uint8_t spatial_layers_ = 1;
    uint8_t temporal_layer_ = 0;
    size_t num_frame_deps_ = 0;
    uint16_t frame_deps_id_diffs_[kMaxNumFrameDependencies];
    int width_ = 0;
    int height_ = 0;
};

class RtpGenericFrameDescriptorExtension00 {
public:
    using value_type = RtpGenericFrameDescriptor;

    static const RTPExtensionType id();

    static const char* uri();

    static uint8_t value_size(const RtpGenericFrameDescriptor& descriptor);

    static bool read_from_buff(bco::Buffer buff, RtpGenericFrameDescriptor& descriptor);

    static bool write_to_buff(bco::Buffer buff, const RtpGenericFrameDescriptor& descriptor);
};

} // namespace brtc

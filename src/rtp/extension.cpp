#include <cassert>
#include <algorithm>
#include "rtp/extension.h"

namespace {

constexpr uint8_t kFlagBeginOfSubframe = 0x80;
constexpr uint8_t kFlagEndOfSubframe = 0x40;

// In version 00, the flags F and L in the first byte correspond to
// kFlagFirstSubframeV00 and kFlagLastSubframeV00. In practice, they were
// always set to |true|.
constexpr uint8_t kFlagFirstSubframeV00 = 0x20;
constexpr uint8_t kFlagLastSubframeV00 = 0x10;

constexpr uint8_t kFlagDependencies = 0x08;
constexpr uint8_t kMaskTemporalLayer = 0x07;

constexpr uint8_t kFlagMoreDependencies = 0x01;
constexpr uint8_t kFlageXtendedOffset = 0x02;
} // namespace
//       0 1 2 3 4 5 6 7
//      +-+-+-+-+-+-+-+-+
//      |B|E|F|L|D|  T  |
//      +-+-+-+-+-+-+-+-+
// B:   |       S       |
//      +-+-+-+-+-+-+-+-+
//      |               |
// B:   +      FID      +
//      |               |
//      +-+-+-+-+-+-+-+-+
//      |               |
//      +     Width     +
// B=1  |               |
// and  +-+-+-+-+-+-+-+-+
// D=0  |               |
//      +     Height    +
//      |               |
//      +-+-+-+-+-+-+-+-+
// D:   |    FDIFF  |X|M|
//      +---------------+
// X:   |      ...      |
//      +-+-+-+-+-+-+-+-+
// M:   |    FDIFF  |X|M|
//      +---------------+
//      |      ...      |
//      +-+-+-+-+-+-+-+-+

namespace brtc {

RtpGenericFrameDescriptor::RtpGenericFrameDescriptor() = default;
RtpGenericFrameDescriptor::RtpGenericFrameDescriptor(
    const RtpGenericFrameDescriptor&)
    = default;
RtpGenericFrameDescriptor::~RtpGenericFrameDescriptor() = default;

int RtpGenericFrameDescriptor::TemporalLayer() const
{
    assert(FirstPacketInSubFrame());
    return temporal_layer_;
}

void RtpGenericFrameDescriptor::SetTemporalLayer(int temporal_layer)
{
    assert(temporal_layer >= 0);
    assert(temporal_layer <= kMaxTemporalLayers);
    temporal_layer_ = temporal_layer;
}

int RtpGenericFrameDescriptor::SpatialLayer() const
{
    assert(FirstPacketInSubFrame());
    int layer = 0;
    uint8_t spatial_layers = spatial_layers_;
    while (spatial_layers_ != 0 && !(spatial_layers & 1)) {
        spatial_layers >>= 1;
        layer++;
    }
    return layer;
}

uint8_t RtpGenericFrameDescriptor::SpatialLayersBitmask() const
{
    assert(FirstPacketInSubFrame());
    return spatial_layers_;
}

void RtpGenericFrameDescriptor::SetSpatialLayersBitmask(
    uint8_t spatial_layers)
{
    assert(FirstPacketInSubFrame());
    spatial_layers_ = spatial_layers;
}

void RtpGenericFrameDescriptor::SetResolution(int width, int height)
{
    assert(FirstPacketInSubFrame());
    assert(width >= 0);
    assert(width <= 0xFFFF);
    assert(height >= 0);
    assert(height <= 0xFFFF);
    width_ = width;
    height_ = height;
}

uint16_t RtpGenericFrameDescriptor::FrameId() const
{
    assert(FirstPacketInSubFrame());
    return frame_id_;
}

void RtpGenericFrameDescriptor::SetFrameId(uint16_t frame_id)
{
    assert(FirstPacketInSubFrame());
    frame_id_ = frame_id;
}

std::span<const uint16_t>
RtpGenericFrameDescriptor::FrameDependenciesDiffs() const
{
    assert(FirstPacketInSubFrame());
    return std::span(frame_deps_id_diffs_, num_frame_deps_);
}

bool RtpGenericFrameDescriptor::AddFrameDependencyDiff(uint16_t fdiff)
{
    assert(FirstPacketInSubFrame());
    if (num_frame_deps_ == kMaxNumFrameDependencies)
        return false;
    if (fdiff == 0)
        return false;
    assert(fdiff <= 1 << 14);
    assert(fdiff >= 0);
    frame_deps_id_diffs_[num_frame_deps_] = fdiff;
    num_frame_deps_++;
    return true;
}

const RTPExtensionType RtpGenericFrameDescriptorExtension00::id()
{
    return RTPExtensionType::kRtpExtensionGenericFrameDescriptor00;
}

const char* RtpGenericFrameDescriptorExtension00::uri()
{
    return "http://www.webrtc.org/experiments/rtp-hdrext/"
           "generic-frame-descriptor-00";
}

uint8_t RtpGenericFrameDescriptorExtension00::value_size(const RtpGenericFrameDescriptor& descriptor)
{
    if (!descriptor.FirstPacketInSubFrame())
        return 1;

    size_t size = 4;
    for (uint16_t fdiff : descriptor.FrameDependenciesDiffs()) {
        size += (fdiff >= (1 << 6)) ? 2 : 1;
    }
    if (descriptor.FirstPacketInSubFrame() && descriptor.FrameDependenciesDiffs().empty() && descriptor.Width() > 0 && descriptor.Height() > 0) {
        size += 4;
    }
    return size;
}

bool RtpGenericFrameDescriptorExtension00::read_from_buff(bco::Buffer buff, RtpGenericFrameDescriptor& descriptor)
{
    if (buff.size() == 0)
        return false;
    auto data = buff.data()[0];

    if (data.empty()) {
        return false;
    }

    bool begins_subframe = (data[0] & kFlagBeginOfSubframe) != 0;
    descriptor.SetFirstPacketInSubFrame(begins_subframe);
    descriptor.SetLastPacketInSubFrame((data[0] & kFlagEndOfSubframe) != 0);

    // Parse Subframe details provided in 1st packet of subframe.
    if (!begins_subframe) {
        return data.size() == 1;
    }
    if (data.size() < 4) {
        return false;
    }
    descriptor.SetTemporalLayer(data[0] & kMaskTemporalLayer);
    descriptor.SetSpatialLayersBitmask(data[1]);
    descriptor.SetFrameId(data[2] | (data[3] << 8));

    // Parse dependencies.
    descriptor.ClearFrameDependencies();
    size_t offset = 4;
    bool has_more_dependencies = (data[0] & kFlagDependencies) != 0;
    if (!has_more_dependencies && data.size() >= offset + 4) {
        uint16_t width = (data[offset] << 8) | data[offset + 1];
        uint16_t height = (data[offset + 2] << 8) | data[offset + 3];
        descriptor.SetResolution(width, height);
        offset += 4;
    }
    while (has_more_dependencies) {
        if (data.size() == offset)
            return false;
        has_more_dependencies = (data[offset] & kFlagMoreDependencies) != 0;
        bool extended = (data[offset] & kFlageXtendedOffset) != 0;
        uint16_t fdiff = data[offset] >> 2;
        offset++;
        if (extended) {
            if (data.size() == offset)
                return false;
            fdiff |= (data[offset] << 6);
            offset++;
        }
        if (!descriptor.AddFrameDependencyDiff(fdiff))
            return false;
    }
    return true;
}

bool RtpGenericFrameDescriptorExtension00::write_to_buff(bco::Buffer buff, const RtpGenericFrameDescriptor& descriptor)
{
    if (buff.size() == 0) {
        return false;
    }
    auto data = buff.data()[0];
    if (data.size() != value_size(descriptor)) {
        std::abort();
    }
    uint8_t base_header = (descriptor.FirstPacketInSubFrame() ? kFlagBeginOfSubframe : 0) | (descriptor.LastPacketInSubFrame() ? kFlagEndOfSubframe : 0);
    base_header |= kFlagFirstSubframeV00;
    base_header |= kFlagLastSubframeV00;

    if (!descriptor.FirstPacketInSubFrame()) {
        data[0] = base_header;
        return true;
    }
    data[0] = base_header | (descriptor.FrameDependenciesDiffs().empty() ? 0 : kFlagDependencies) | descriptor.TemporalLayer();
    data[1] = descriptor.SpatialLayersBitmask();
    uint16_t frame_id = descriptor.FrameId();
    data[2] = frame_id & 0xff;
    data[3] = frame_id >> 8;
    std::span<const uint16_t> fdiffs = descriptor.FrameDependenciesDiffs();
    size_t offset = 4;
    if (descriptor.FirstPacketInSubFrame() && fdiffs.empty() && descriptor.Width() > 0 && descriptor.Height() > 0) {
        data[offset++] = (descriptor.Width() >> 8);
        data[offset++] = (descriptor.Width() & 0xFF);
        data[offset++] = (descriptor.Height() >> 8);
        data[offset++] = (descriptor.Height() & 0xFF);
    }
    for (size_t i = 0; i < fdiffs.size(); i++) {
        bool extended = fdiffs[i] >= (1 << 6);
        bool more = i < fdiffs.size() - 1;
        data[offset++] = ((fdiffs[i] & 0x3f) << 2) | (extended ? kFlageXtendedOffset : 0) | (more ? kFlagMoreDependencies : 0);
        if (extended) {
            data[offset++] = fdiffs[i] >> 6;
        }
    }
    return true;
}

} // namespace brtc

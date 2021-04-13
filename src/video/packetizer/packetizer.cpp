#include "video/packetizer/packetizer.h"
#include "video/packetizer/packetizer_h264.h"

namespace brtc {

std::unique_ptr<Packetizer> Packetizer::create(Frame decoded_frame, VideoCodecType codec_type, PayloadSizeLimits limits)
{
    switch (codec_type) {
    case brtc::VideoCodecType::H264:
        return std::make_unique<PacketizerH264>(decoded_frame, limits);
    case brtc::VideoCodecType::H265:
        return nullptr;
    case brtc::VideoCodecType::VP8:
        return nullptr;
    case brtc::VideoCodecType::VP9:
        return nullptr;
    default:
        assert(false);
    }
    return nullptr;
}

} // namespace brtc

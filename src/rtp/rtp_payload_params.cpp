/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <stddef.h>
#include <optional>
#include <algorithm>

#include <glog/logging.h>

#include <brtc/video/codec_interface.h>
#include <brtc/video/common_constants.h>

#include "rtp/rtp.h"
#include "rtp/rtp_payload_params.h"

namespace webrtc {

namespace {
void PopulateRtpWithCodecSpecifics(const CodecSpecificInfo& info,
                                   std::optional<int> spatial_index,
                                   brtc::UnionRTPVideoHeader* rtp) {
  auto& header = std::get<brtc::RTPVideoHeader>(*rtp);
  header.codec = info.codecType;
  header.is_last_frame_in_picture = info.end_of_picture;
  switch (info.codecType) {
    case brtc::VideoCodecType::VP8: {
      auto& vp8_header = std::get<brtc::RTPVideoHeaderVP8>(*rtp);
      vp8_header.InitRTPVideoHeaderVP8();
      vp8_header.nonReference = info.codecSpecific.VP8.nonReference;
      vp8_header.temporalIdx = info.codecSpecific.VP8.temporalIdx;
      vp8_header.layerSync = info.codecSpecific.VP8.layerSync;
      vp8_header.keyIdx = info.codecSpecific.VP8.keyIdx;
      header.simulcastIdx = spatial_index.value_or(0);
      return;
    }
    case brtc::VideoCodecType::VP9: {
        auto& vp9_header = std::get<brtc::RTPVideoHeaderVP9>(*rtp);
      vp9_header.InitRTPVideoHeaderVP9();
      vp9_header.inter_pic_predicted =
          info.codecSpecific.VP9.inter_pic_predicted;
      vp9_header.flexible_mode = info.codecSpecific.VP9.flexible_mode;
      vp9_header.ss_data_available = info.codecSpecific.VP9.ss_data_available;
      vp9_header.non_ref_for_inter_layer_pred =
          info.codecSpecific.VP9.non_ref_for_inter_layer_pred;
      vp9_header.temporal_idx = info.codecSpecific.VP9.temporal_idx;
      vp9_header.temporal_up_switch = info.codecSpecific.VP9.temporal_up_switch;
      vp9_header.inter_layer_predicted =
          info.codecSpecific.VP9.inter_layer_predicted;
      vp9_header.gof_idx = info.codecSpecific.VP9.gof_idx;
      vp9_header.num_spatial_layers = info.codecSpecific.VP9.num_spatial_layers;
      vp9_header.first_active_layer = info.codecSpecific.VP9.first_active_layer;
      if (vp9_header.num_spatial_layers > 1) {
        vp9_header.spatial_idx = spatial_index.value_or(kNoSpatialIdx);
      } else {
        vp9_header.spatial_idx = kNoSpatialIdx;
      }
      if (info.codecSpecific.VP9.ss_data_available) {
        vp9_header.spatial_layer_resolution_present =
            info.codecSpecific.VP9.spatial_layer_resolution_present;
        if (info.codecSpecific.VP9.spatial_layer_resolution_present) {
          for (size_t i = 0; i < info.codecSpecific.VP9.num_spatial_layers;
               ++i) {
            vp9_header._width[i] = info.codecSpecific.VP9.width[i];
            vp9_header._height[i] = info.codecSpecific.VP9.height[i];
          }
        }
        vp9_header.gof.CopyGofInfoVP9(info.codecSpecific.VP9.gof);
      }

      vp9_header.num_ref_pics = info.codecSpecific.VP9.num_ref_pics;
      for (int i = 0; i < info.codecSpecific.VP9.num_ref_pics; ++i) {
        vp9_header.pid_diff[i] = info.codecSpecific.VP9.p_diff[i];
      }
      vp9_header.end_of_picture = info.end_of_picture;
      return;
    }
    case brtc::VideoCodecType::H264: {
        auto& h264_header = std::get<brtc::RTPVideoHeaderH264>(*rtp);
      //h264_header.packetization_mode =
      //    info.codecSpecific.H264.packetization_mode;
      h264_header.simulcastIdx = spatial_index.value_or(0);
      return;
    }
    //case kVideoCodecMultiplex:
    //case kVideoCodecGeneric:
    //  rtp->codec = kVideoCodecGeneric;
    //  rtp->simulcastIdx = spatial_index.value_or(0);
    //  return;
    default:
      return;
  }
}

//void SetVideoTiming(const EncodedImage& image, VideoSendTiming* timing) {
//  if (image.timing_.flags == VideoSendTiming::TimingFrameFlags::kInvalid ||
//      image.timing_.flags == VideoSendTiming::TimingFrameFlags::kNotTriggered) {
//    timing->flags = VideoSendTiming::TimingFrameFlags::kInvalid;
//    return;
//  }
//
//  timing->encode_start_delta_ms = VideoSendTiming::GetDeltaCappedMs(
//      image.capture_time_ms_, image.timing_.encode_start_ms);
//  timing->encode_finish_delta_ms = VideoSendTiming::GetDeltaCappedMs(
//      image.capture_time_ms_, image.timing_.encode_finish_ms);
//  timing->packetization_finish_delta_ms = 0;
//  timing->pacer_exit_delta_ms = 0;
//  timing->network_timestamp_delta_ms = 0;
//  timing->network2_timestamp_delta_ms = 0;
//  timing->flags = image.timing_.flags;
//}
}  // namespace

namespace {
inline bool StartsWith(std::string_view text, std::string_view prefix) noexcept
{
    return prefix.empty() || (text.size() >= prefix.size() && memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}
}

RtpPayloadParams::RtpPayloadParams(const uint32_t ssrc,
                                   const RtpPayloadState* state,
                                   const WebRtcKeyValueConfig& trials)
    : ssrc_(ssrc),
      generic_picture_id_experiment_(
          StartsWith(trials.Lookup("WebRTC-GenericPictureId"),
                           "Enabled")) {
  for (auto& spatial_layer : last_shared_frame_id_)
    spatial_layer.fill(-1);

  buffer_id_to_frame_id_.fill(-1);

  //Random random(rtc::TimeMicros());
  //state_.picture_id =
  //    state ? state->picture_id : (random.Rand<int16_t>() & 0x7FFF);
  state_.picture_id = state ? state->picture_id : (int16_t)::rand() & 0x7FFF;
  state_.tl0_pic_idx = state ? state->tl0_pic_idx : ((uint8_t)::rand());
}

RtpPayloadParams::RtpPayloadParams(const RtpPayloadParams& other) = default;

RtpPayloadParams::~RtpPayloadParams() {}

brtc::UnionRTPVideoHeader RtpPayloadParams::GetRtpVideoHeader(
    const brtc::EncodedFrame& image,
    const CodecSpecificInfo* codec_specific_info,
    int64_t shared_frame_id) {
  brtc::UnionRTPVideoHeader rtp_video_header;
  if (codec_specific_info) {
    PopulateRtpWithCodecSpecifics(*codec_specific_info, image.spatial_index,
                                  &rtp_video_header);
  }
  auto& header = std::get<brtc::RTPVideoHeader>(rtp_video_header);
  header.frame_type = image.video_frame_type; // FIXME: key or delta
  //header.rotation = image.rotation_;
  //header.content_type = image.content_type_;
  //header.playout_delay = image.playout_delay_;
  header.width = image.width;
  header.height = image.height;
  header.color_space = image.color_space;
  //header.video_frame_tracking_id = image.VideoFrameTrackingId();
  //SetVideoTiming(image, &header.video_timing);

  const bool is_keyframe = image.video_frame_type == brtc::VideoFrameType::VideoFrameKey;
  const bool first_frame_in_picture =
      (codec_specific_info && codec_specific_info->codecType == brtc::VideoCodecType::VP9)
          ? codec_specific_info->codecSpecific.VP9.first_frame_in_picture
          : true;

  SetCodecSpecific(&rtp_video_header, first_frame_in_picture);

  SetGeneric(codec_specific_info, shared_frame_id, is_keyframe,
             &rtp_video_header);

  return rtp_video_header;
}

uint32_t RtpPayloadParams::ssrc() const {
  return ssrc_;
}

RtpPayloadState RtpPayloadParams::state() const {
  return state_;
}

void RtpPayloadParams::SetCodecSpecific(brtc::UnionRTPVideoHeader* rtp_video_header,
                                        bool first_frame_in_picture) {
  // Always set picture id. Set tl0_pic_idx iff temporal index is set.
  if (first_frame_in_picture) {
    state_.picture_id = (static_cast<uint16_t>(state_.picture_id) + 1) & 0x7FFF;
  }
  auto& header = std::get<brtc::RTPVideoHeader>(*rtp_video_header);
  if (header.codec == brtc::VideoCodecType::VP8) {
    auto& vp8_header = std::get<brtc::RTPVideoHeaderVP8>(*rtp_video_header);
    vp8_header.pictureId = state_.picture_id;

    if (vp8_header.temporalIdx != kNoTemporalIdx) {
      if (vp8_header.temporalIdx == 0) {
        ++state_.tl0_pic_idx;
      }
      vp8_header.tl0PicIdx = state_.tl0_pic_idx;
    }
  }
  if (header.codec == brtc::VideoCodecType::VP9) {
    auto& vp9_header = std::get<brtc::RTPVideoHeaderVP9>(*rtp_video_header);
    vp9_header.picture_id = state_.picture_id;

    // Note that in the case that we have no temporal layers but we do have
    // spatial layers, packets will carry layering info with a temporal_idx of
    // zero, and we then have to set and increment tl0_pic_idx.
    if (vp9_header.temporal_idx != kNoTemporalIdx ||
        vp9_header.spatial_idx != kNoSpatialIdx) {
      if (first_frame_in_picture &&
          (vp9_header.temporal_idx == 0 ||
           vp9_header.temporal_idx == kNoTemporalIdx)) {
        ++state_.tl0_pic_idx;
      }
      vp9_header.tl0_pic_idx = state_.tl0_pic_idx;
    }
  }
  //if (generic_picture_id_experiment_ &&
  //    header.codec == kVideoCodecGeneric) {
  //  rtp_video_header->video_type_header.emplace<RTPVideoHeaderLegacyGeneric>()
  //      .picture_id = state_.picture_id;
  //}
}

brtc::RTPVideoHeader::GenericDescriptorInfo
RtpPayloadParams::GenericDescriptorFromFrameInfo(
    const GenericFrameInfo& frame_info,
    int64_t frame_id) {
  brtc::RTPVideoHeader::GenericDescriptorInfo generic;
  generic.frame_id = frame_id;
  generic.dependencies = dependencies_calculator_.FromBuffersUsage(
      frame_id, frame_info.encoder_buffers);
  generic.chain_diffs =
      chains_calculator_.From(frame_id, frame_info.part_of_chain);
  generic.spatial_index = frame_info.spatial_id;
  generic.temporal_index = frame_info.temporal_id;
  generic.decode_target_indications = frame_info.decode_target_indications;
  generic.active_decode_targets = frame_info.active_decode_targets;
  return generic;
}

void RtpPayloadParams::SetGeneric(const CodecSpecificInfo* codec_specific_info,
                                  int64_t frame_id,
                                  bool is_keyframe,
                                  brtc::UnionRTPVideoHeader* rtp_video_header) {
  if (codec_specific_info && codec_specific_info->generic_frame_info &&
      !codec_specific_info->generic_frame_info->encoder_buffers.empty()) {
    if (is_keyframe) {
      // Key frame resets all chains it is in.
      chains_calculator_.Reset(
          codec_specific_info->generic_frame_info->part_of_chain);
    }
    auto& header = std::get<brtc::RTPVideoHeader>(*rtp_video_header);
    header.generic = GenericDescriptorFromFrameInfo(
        *codec_specific_info->generic_frame_info, frame_id);
    return;
  }
  auto& header = std::get<brtc::RTPVideoHeader>(*rtp_video_header);
  switch (header.codec) {
    //case brtc::VideoCodecType::kVideoCodecGeneric:
    //  GenericToGeneric(frame_id, is_keyframe, rtp_video_header);
    //  return;
    case brtc::VideoCodecType::VP8:
      if (codec_specific_info) {
        Vp8ToGeneric(codec_specific_info->codecSpecific.VP8, frame_id,
                     is_keyframe, rtp_video_header);
      }
      return;
    case brtc::VideoCodecType::VP9:
    //case VideoCodecType::kVideoCodecAV1:
      // TODO(philipel): Implement VP9 and AV1 to generic descriptor.
      return;
    case brtc::VideoCodecType::H264:
      if (codec_specific_info) {
        H264ToGeneric(codec_specific_info->codecSpecific.H264, frame_id,
                      is_keyframe, rtp_video_header);
      }
      return;
    //case VideoCodecType::kVideoCodecMultiplex:
    //  return;
  }
  assert(false);
  //RTC_NOTREACHED() << "Unsupported codec.";
}

void RtpPayloadParams::GenericToGeneric(int64_t shared_frame_id,
                                        bool is_keyframe,
                                        brtc::UnionRTPVideoHeader* rtp_video_header) {
  auto& header = std::get<brtc::RTPVideoHeader>(*rtp_video_header);
  brtc::RTPVideoHeader::GenericDescriptorInfo& generic =
      header.generic.emplace();

  generic.frame_id = shared_frame_id;

  if (is_keyframe) {
    last_shared_frame_id_[0].fill(-1);
  } else {
    int64_t frame_id = last_shared_frame_id_[0][0];
    assert(frame_id != -1);
    assert(frame_id < shared_frame_id);
    generic.dependencies.push_back(frame_id);
  }

  last_shared_frame_id_[0][0] = shared_frame_id;
}

void RtpPayloadParams::H264ToGeneric(const CodecSpecificInfoH264& h264_info,
                                     int64_t shared_frame_id,
                                     bool is_keyframe,
                                     brtc::UnionRTPVideoHeader* rtp_video_header) {
  const int temporal_index =
      h264_info.temporal_idx != kNoTemporalIdx ? h264_info.temporal_idx : 0;

  if (temporal_index >= brtc::RtpGenericFrameDescriptor::kMaxTemporalLayers) {
    LOG(WARNING) << "Temporal and/or spatial index is too high to be "
                           "used with generic frame descriptor.";
    return;
  }
  auto& header = std::get<brtc::RTPVideoHeader>(*rtp_video_header);
  brtc::RTPVideoHeader::GenericDescriptorInfo& generic =
      header.generic.emplace();

  generic.frame_id = shared_frame_id;
  generic.temporal_index = temporal_index;

  if (is_keyframe) {
    assert(temporal_index == 0);
    last_shared_frame_id_[/*spatial index*/ 0].fill(-1);
    last_shared_frame_id_[/*spatial index*/ 0][temporal_index] =
        shared_frame_id;
    return;
  }

  if (h264_info.base_layer_sync) {
    int64_t tl0_frame_id = last_shared_frame_id_[/*spatial index*/ 0][0];

    for (int i = 1; i < brtc::RtpGenericFrameDescriptor::kMaxTemporalLayers; ++i) {
      if (last_shared_frame_id_[/*spatial index*/ 0][i] < tl0_frame_id) {
        last_shared_frame_id_[/*spatial index*/ 0][i] = -1;
      }
    }

    assert(tl0_frame_id >= 0);
    assert(tl0_frame_id < shared_frame_id);
    generic.dependencies.push_back(tl0_frame_id);
  } else {
    for (int i = 0; i <= temporal_index; ++i) {
      int64_t frame_id = last_shared_frame_id_[/*spatial index*/ 0][i];

      if (frame_id != -1) {
        assert(frame_id < shared_frame_id);
        generic.dependencies.push_back(frame_id);
      }
    }
  }

  last_shared_frame_id_[/*spatial_index*/ 0][temporal_index] = shared_frame_id;
}

void RtpPayloadParams::Vp8ToGeneric(const CodecSpecificInfoVP8& vp8_info,
                                    int64_t shared_frame_id,
                                    bool is_keyframe,
                                    brtc::UnionRTPVideoHeader* rtp_video_header) {
  const auto& vp8_header = std::get<brtc::RTPVideoHeaderVP8>(*rtp_video_header);
  const int spatial_index = 0;
  const int temporal_index =
      vp8_header.temporalIdx != kNoTemporalIdx ? vp8_header.temporalIdx : 0;

  if (temporal_index >= brtc::RtpGenericFrameDescriptor::kMaxTemporalLayers ||
      spatial_index >= brtc::RtpGenericFrameDescriptor::kMaxSpatialLayers) {
    LOG(WARNING) << "Temporal and/or spatial index is too high to be "
                           "used with generic frame descriptor.";
    return;
  }
  auto& header = std::get<brtc::RTPVideoHeader>(*rtp_video_header);
  brtc::RTPVideoHeader::GenericDescriptorInfo& generic =
      header.generic.emplace();

  generic.frame_id = shared_frame_id;
  generic.spatial_index = spatial_index;
  generic.temporal_index = temporal_index;

  if (vp8_info.useExplicitDependencies) {
    SetDependenciesVp8New(vp8_info, shared_frame_id, is_keyframe,
                          vp8_header.layerSync, &generic);
  } else {
    SetDependenciesVp8Deprecated(vp8_info, shared_frame_id, is_keyframe,
                                 spatial_index, temporal_index,
                                 vp8_header.layerSync, &generic);
  }
}

void RtpPayloadParams::SetDependenciesVp8Deprecated(
    const CodecSpecificInfoVP8& vp8_info,
    int64_t shared_frame_id,
    bool is_keyframe,
    int spatial_index,
    int temporal_index,
    bool layer_sync,
    brtc::RTPVideoHeader::GenericDescriptorInfo* generic) {
  assert(!vp8_info.useExplicitDependencies);
  assert(!new_version_used_.has_value() || !new_version_used_.value());
  new_version_used_ = false;

  if (is_keyframe) {
    assert(temporal_index = 0);
    last_shared_frame_id_[spatial_index].fill(-1);
    last_shared_frame_id_[spatial_index][temporal_index] = shared_frame_id;
    return;
  }

  if (layer_sync) {
    int64_t tl0_frame_id = last_shared_frame_id_[spatial_index][0];

    for (int i = 1; i < brtc::RtpGenericFrameDescriptor::kMaxTemporalLayers; ++i) {
      if (last_shared_frame_id_[spatial_index][i] < tl0_frame_id) {
        last_shared_frame_id_[spatial_index][i] = -1;
      }
    }

    assert(tl0_frame_id >= 0);
    assert(tl0_frame_id < shared_frame_id);
    generic->dependencies.push_back(tl0_frame_id);
  } else {
    for (int i = 0; i <= temporal_index; ++i) {
      int64_t frame_id = last_shared_frame_id_[spatial_index][i];

      if (frame_id != -1) {
        assert(frame_id < shared_frame_id);
        generic->dependencies.push_back(frame_id);
      }
    }
  }

  last_shared_frame_id_[spatial_index][temporal_index] = shared_frame_id;
}

void RtpPayloadParams::SetDependenciesVp8New(
    const CodecSpecificInfoVP8& vp8_info,
    int64_t shared_frame_id,
    bool is_keyframe,
    bool layer_sync,
    brtc::RTPVideoHeader::GenericDescriptorInfo* generic) {
  assert(vp8_info.useExplicitDependencies);
  assert(!new_version_used_.has_value() || new_version_used_.value());
  new_version_used_ = true;

  if (is_keyframe) {
    assert(vp8_info.referencedBuffersCount == 0u);
    buffer_id_to_frame_id_.fill(shared_frame_id);
    return;
  }

  constexpr size_t kBuffersCountVp8 = CodecSpecificInfoVP8::kBuffersCount;

  assert(vp8_info.referencedBuffersCount > 0u);
  assert(vp8_info.referencedBuffersCount <
                std::size(vp8_info.referencedBuffers)); // Am I wrong?

  for (size_t i = 0; i < vp8_info.referencedBuffersCount; ++i) {
    const size_t referenced_buffer = vp8_info.referencedBuffers[i];
    assert(referenced_buffer < kBuffersCountVp8);
    assert(referenced_buffer < buffer_id_to_frame_id_.size());

    const int64_t dependency_frame_id =
        buffer_id_to_frame_id_[referenced_buffer];
    assert(dependency_frame_id >= 0);
    assert(dependency_frame_id < shared_frame_id);

    const bool is_new_dependency =
        std::find(generic->dependencies.begin(), generic->dependencies.end(),
                  dependency_frame_id) == generic->dependencies.end();
    if (is_new_dependency) {
      generic->dependencies.push_back(dependency_frame_id);
    }
  }

  assert(vp8_info.updatedBuffersCount <= kBuffersCountVp8);
  for (size_t i = 0; i < vp8_info.updatedBuffersCount; ++i) {
    const size_t updated_id = vp8_info.updatedBuffers[i];
    buffer_id_to_frame_id_[updated_id] = shared_frame_id;
  }

  assert(buffer_id_to_frame_id_.size() <= kBuffersCountVp8);
}

}  // namespace webrtc

/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once

#include <array>
#include <optional>

#include <brtc/webrtc_key_value_config.h>
#include <brtc/video/generic_frame_info.h>
#include "rtp/rtp.h"
#include "rtp/rtp_config.h"
#include "rtp/extension.h"
#include "video/frame_dependencies_calculator.h"
#include "video/chain_diff_calculator.h"

namespace webrtc {

class RtpRtcp;

// State for setting picture id and tl0 pic idx, for VP8 and VP9
// TODO(nisse): Make these properties not codec specific.
class RtpPayloadParams final {
 public:
  RtpPayloadParams(const uint32_t ssrc,
                   const RtpPayloadState* state,
                   const WebRtcKeyValueConfig& trials);
  RtpPayloadParams(const RtpPayloadParams& other);
  ~RtpPayloadParams();

  brtc::UnionRTPVideoHeader GetRtpVideoHeader(const brtc::EncodedFrame& image,
                                   const CodecSpecificInfo* codec_specific_info,
                                   int64_t shared_frame_id);

  uint32_t ssrc() const;

  RtpPayloadState state() const;

 private:
  void SetCodecSpecific(brtc::UnionRTPVideoHeader* rtp_video_header,
                        bool first_frame_in_picture);
  brtc::RTPVideoHeader::GenericDescriptorInfo GenericDescriptorFromFrameInfo(
      const GenericFrameInfo& frame_info,
      int64_t frame_id);
  void SetGeneric(const CodecSpecificInfo* codec_specific_info,
                  int64_t frame_id,
                  bool is_keyframe,
                  brtc::UnionRTPVideoHeader* rtp_video_header);

  void Vp8ToGeneric(const CodecSpecificInfoVP8& vp8_info,
                    int64_t shared_frame_id,
                    bool is_keyframe,
                    brtc::UnionRTPVideoHeader* rtp_video_header);

  void H264ToGeneric(const CodecSpecificInfoH264& h264_info,
                     int64_t shared_frame_id,
                     bool is_keyframe,
                     brtc::UnionRTPVideoHeader* rtp_video_header);

  void GenericToGeneric(int64_t shared_frame_id,
                        bool is_keyframe,
                        brtc::UnionRTPVideoHeader* rtp_video_header);

  // TODO(bugs.webrtc.org/10242): Delete SetDependenciesVp8Deprecated() and move
  // the logic in SetDependenciesVp8New() into Vp8ToGeneric() once all hardware
  // wrappers have been updated.
  void SetDependenciesVp8Deprecated(
      const CodecSpecificInfoVP8& vp8_info,
      int64_t shared_frame_id,
      bool is_keyframe,
      int spatial_index,
      int temporal_index,
      bool layer_sync,
      brtc::RTPVideoHeader::GenericDescriptorInfo* generic);
  void SetDependenciesVp8New(const CodecSpecificInfoVP8& vp8_info,
                             int64_t shared_frame_id,
                             bool is_keyframe,
                             bool layer_sync,
                             brtc::RTPVideoHeader::GenericDescriptorInfo* generic);

  FrameDependenciesCalculator dependencies_calculator_;
  ChainDiffCalculator chains_calculator_;
  // TODO(bugs.webrtc.org/10242): Remove once all encoder-wrappers are updated.
  // Holds the last shared frame id for a given (spatial, temporal) layer.
  std::array<std::array<int64_t, brtc::RtpGenericFrameDescriptor::kMaxTemporalLayers>,
             brtc::RtpGenericFrameDescriptor::kMaxSpatialLayers>
      last_shared_frame_id_;

  // TODO(eladalon): When additional codecs are supported,
  // set kMaxCodecBuffersCount to the max() of these codecs' buffer count.
  static constexpr size_t kMaxCodecBuffersCount =
      CodecSpecificInfoVP8::kBuffersCount;

  // Maps buffer IDs to the frame-ID stored in them.
  std::array<int64_t, kMaxCodecBuffersCount> buffer_id_to_frame_id_;

  // Until we remove SetDependenciesVp8Deprecated(), we should make sure
  // that, for a given object, we either always use
  // SetDependenciesVp8Deprecated(), or always use SetDependenciesVp8New().
  // TODO(bugs.webrtc.org/10242): Remove.
  std::optional<bool> new_version_used_;

  const uint32_t ssrc_;
  RtpPayloadState state_;

  const bool generic_picture_id_experiment_;
};
}  // namespace webrtc

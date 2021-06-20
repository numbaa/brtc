/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once

#include <optional>
#include "video/depacketizer/video_rtp_depacketizer.h"

namespace webrtc {
class VideoRtpDepacketizerH264 : public VideoRtpDepacketizer {
 public:
  ~VideoRtpDepacketizerH264() override = default;

  void Parse(brtc::RtpPacket& packet) override;
};
}  // namespace webrtc

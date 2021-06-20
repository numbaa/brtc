/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once

#include <cstdint>
#include "rtp/rtp.h"


namespace webrtc {

class VideoRtpDepacketizer {
 public:

  virtual ~VideoRtpDepacketizer() = default;
  virtual void Parse(brtc::RtpPacket& packet) = 0;
  //virtual rtc::scoped_refptr<EncodedImageBuffer> AssembleFrame(
  //    rtc::ArrayView<const rtc::ArrayView<const uint8_t>> rtp_payloads);
};

}  // namespace webrtc


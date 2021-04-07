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

#include <memory>

#include <brtc/frame.h>
#include "reference_finder.h"
#include "../../common/sequence_number_util.h"

namespace brtc {

class RtpFrameIdOnlyRefFinder {
 public:
  RtpFrameIdOnlyRefFinder() = default;

  RtpFrameReferenceFinder::ReturnVector ManageFrame(
      std::unique_ptr<ReceivedFrame> frame,
      int frame_id);

 private:
  static constexpr int kFrameIdLength = 1 << 15;
  webrtc::SeqNumUnwrapper<uint16_t, kFrameIdLength> unwrapper_;
};

}  // namespace brtc


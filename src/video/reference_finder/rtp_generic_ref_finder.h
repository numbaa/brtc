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
#include "video/reference_finder/reference_finder.h"

namespace webrtc {

class RtpGenericFrameRefFinder {
 public:
  RtpGenericFrameRefFinder() = default;

  brtc::RtpFrameReferenceFinder::ReturnVector ManageFrame(
      std::unique_ptr<brtc::ReceivedFrame> frame,
      const brtc::RTPVideoHeader::GenericDescriptorInfo& descriptor);
};

}  // namespace webrtc


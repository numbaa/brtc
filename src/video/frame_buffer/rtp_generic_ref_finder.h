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

namespace brtc {

class RtpGenericFrameRefFinder {
 public:
  RtpGenericFrameRefFinder() = default;

  RtpFrameReferenceFinder::ReturnVector ManageFrame(
      std::unique_ptr<Frame> frame,
      const RTPVideoHeader::GenericDescriptorInfo& descriptor);
};

}  // namespace brtc


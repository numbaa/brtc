/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <utility>
#include "video/reference_finder/rtp_frame_id_only_ref_finder.h"

namespace webrtc {

brtc::RtpFrameReferenceFinder::ReturnVector RtpFrameIdOnlyRefFinder::ManageFrame(
    std::unique_ptr<brtc::ReceivedFrame> frame,
    int frame_id) {
  frame->spatial_index = 0;
  frame->id = unwrapper_.Unwrap(frame_id & (kFrameIdLength - 1));
  frame->num_references =
      frame->frame_type == brtc::VideoFrameType::VideoFrameKey ? 0 : 1;
  frame->references[0] = frame->id - 1;

  brtc::RtpFrameReferenceFinder::ReturnVector res;
  res.push_back(std::move(frame));
  return res;
}

}  // namespace webrtc

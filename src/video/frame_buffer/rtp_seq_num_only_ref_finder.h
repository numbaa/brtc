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

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include <brtc/frame.h>
#include "reference_finder.h"
#include "../../common/sequence_number_util.h"

namespace brtc {

class RtpSeqNumOnlyRefFinder {
 public:
  RtpSeqNumOnlyRefFinder() = default;

  RtpFrameReferenceFinder::ReturnVector ManageFrame(
      std::unique_ptr<ReceivedFrame> frame);
  RtpFrameReferenceFinder::ReturnVector PaddingReceived(uint16_t seq_num);
  void ClearTo(uint16_t seq_num);

 private:
  static constexpr int kMaxStashedFrames = 100;
  static constexpr int kMaxPaddingAge = 100;

  enum FrameDecision { kStash, kHandOff, kDrop };

  FrameDecision ManageFrameInternal(ReceivedFrame* frame);
  void RetryStashedFrames(RtpFrameReferenceFinder::ReturnVector& res);
  void UpdateLastPictureIdWithPadding(uint16_t seq_num);

  // For every group of pictures, hold two sequence numbers. The first being
  // the sequence number of the last packet of the last completed frame, and
  // the second being the sequence number of the last packet of the last
  // completed frame advanced by any potential continuous packets of padding.
  std::map<uint16_t,
           std::pair<uint16_t, uint16_t>,
           webrtc::DescendingSeqNumComp<uint16_t>>
      last_seq_num_gop_;

  // Padding packets that have been received but that are not yet continuous
  // with any group of pictures.
  std::set<uint16_t, webrtc::DescendingSeqNumComp<uint16_t>> stashed_padding_;

  // Frames that have been fully received but didn't have all the information
  // needed to determine their references.
  std::deque<std::unique_ptr<ReceivedFrame>> stashed_frames_;

  // Unwrapper used to unwrap generic RTP streams. In a generic stream we derive
  // a picture id from the packet sequence number.
  webrtc::SeqNumUnwrapper<uint16_t> rtp_seq_num_unwrapper_;
};

}  // namespace brtc


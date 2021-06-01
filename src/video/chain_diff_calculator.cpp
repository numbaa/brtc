/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "video/chain_diff_calculator.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <vector>
#include <glog/logging.h>

namespace webrtc {

void ChainDiffCalculator::Reset(const std::vector<bool>& chains) {
  last_frame_in_chain_.resize(chains.size());
  for (size_t i = 0; i < chains.size(); ++i) {
    if (chains[i]) {
      last_frame_in_chain_[i] = std::nullopt;
    }
  }
}

std::vector<int> ChainDiffCalculator::ChainDiffs(
    int64_t frame_id) const {
  std::vector<int> result;
  result.reserve(last_frame_in_chain_.size());
  for (const auto& frame_id_in_chain : last_frame_in_chain_) {
      result.push_back(frame_id_in_chain ? static_cast<int>((frame_id - *frame_id_in_chain)) : 0);
  }
  return result;
}

std::vector<int> ChainDiffCalculator::From(
    int64_t frame_id,
    const std::vector<bool>& chains) {
  auto result = ChainDiffs(frame_id);
  if (chains.size() != last_frame_in_chain_.size()) {
    LOG(ERROR) << "Insconsistent chain configuration for frame#"
                      << frame_id << ": expected "
                      << last_frame_in_chain_.size() << " chains, found "
                      << chains.size();
  }
  size_t num_chains = std::min(last_frame_in_chain_.size(), chains.size());
  for (size_t i = 0; i < num_chains; ++i) {
    if (chains[i]) {
      last_frame_in_chain_[i] = frame_id;
    }
  }
  return result;
}

}  // namespace webrtc

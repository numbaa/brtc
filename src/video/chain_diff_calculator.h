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

#include <stdint.h>

#include <vector>
#include <optional>

namespace webrtc {

// This class is thread compatible.
class ChainDiffCalculator {
 public:
  ChainDiffCalculator() = default;
  ChainDiffCalculator(const ChainDiffCalculator&) = default;
  ChainDiffCalculator& operator=(const ChainDiffCalculator&) = default;

  // Restarts chains, i.e. for position where chains[i] == true next chain_diff
  // will be 0. Saves chains.size() as number of chains in the stream.
  void Reset(const std::vector<bool>& chains);

  // Returns chain diffs based on flags if frame is part of the chain.
  std::vector<int> From(int64_t frame_id,
                                   const std::vector<bool>& chains);

 private:
  std::vector<int> ChainDiffs(int64_t frame_id) const;

  std::vector<std::optional<int64_t>> last_frame_in_chain_;
};

}  // namespace webrtc


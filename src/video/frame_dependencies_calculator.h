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
#include <span>
#include <optional>

#include <brtc/video/generic_frame_info.h>

namespace webrtc {

// This class is thread compatible.
class FrameDependenciesCalculator {
 public:
  FrameDependenciesCalculator() = default;
  FrameDependenciesCalculator(const FrameDependenciesCalculator&) = default;
  FrameDependenciesCalculator& operator=(const FrameDependenciesCalculator&) =
      default;

  // Calculates frame dependencies based on previous encoder buffer usage.
  std::vector<int64_t> FromBuffersUsage(
      int64_t frame_id,
      std::span<const CodecBufferUsage> buffers_usage);

 private:
  struct BufferUsage {
    std::optional<int64_t> frame_id;
      std::vector<int64_t> dependencies;
  };

  std::vector<BufferUsage> buffers_;
};

}  // namespace webrtc


/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "video/frame_dependencies_calculator.h"

#include <cassert>
#include <cstdint>

#include <iterator>
#include <set>
#include <algorithm>

#include <glog/logging.h>

namespace webrtc {

std::vector<int64_t> FrameDependenciesCalculator::FromBuffersUsage(
    int64_t frame_id,
    std::span<const CodecBufferUsage> buffers_usage) {
    std::vector<int64_t> dependencies;
  assert(buffers_usage.size() > 0);
  for (const CodecBufferUsage& buffer_usage : buffers_usage) {
    assert(buffer_usage.id >= 0);
    if (buffers_.size() <= static_cast<size_t>(buffer_usage.id)) {
      buffers_.resize(buffer_usage.id + 1);
    }
  }
  std::set<int64_t> direct_depenendencies;
  std::set<int64_t> indirect_depenendencies;

  for (const CodecBufferUsage& buffer_usage : buffers_usage) {
    if (!buffer_usage.referenced) {
      continue;
    }
    const BufferUsage& buffer = buffers_[buffer_usage.id];
    if (buffer.frame_id == std::nullopt) {
      LOG(ERROR) << "Odd configuration: frame " << frame_id
                        << " references buffer #" << buffer_usage.id
                        << " that was never updated.";
      continue;
    }
    direct_depenendencies.insert(*buffer.frame_id);
    indirect_depenendencies.insert(buffer.dependencies.begin(),
                                   buffer.dependencies.end());
  }
  // Reduce references: if frame #3 depends on frame #2 and #1, and frame #2
  // depends on frame #1, then frame #3 needs to depend just on frame #2.
  // Though this set diff removes only 1 level of indirection, it seems
  // enough for all currently used structures.
  std::ranges::set_difference(direct_depenendencies, indirect_depenendencies,
                         std::back_inserter(dependencies));

  // Update buffers.
  for (const CodecBufferUsage& buffer_usage : buffers_usage) {
    if (!buffer_usage.updated) {
      continue;
    }
    BufferUsage& buffer = buffers_[buffer_usage.id];
    buffer.frame_id = frame_id;
    buffer.dependencies.assign(direct_depenendencies.begin(),
                               direct_depenendencies.end());
  }

  return dependencies;
}

}  // namespace webrtc

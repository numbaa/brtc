/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <cassert>
#include <string_view>
#include <brtc/video/dependency_descriptor.h>

namespace webrtc {

constexpr int DependencyDescriptor::kMaxSpatialIds;
constexpr int DependencyDescriptor::kMaxTemporalIds;
constexpr int DependencyDescriptor::kMaxTemplates;
constexpr int DependencyDescriptor::kMaxDecodeTargets;

namespace webrtc_impl {

std::vector<DecodeTargetIndication> StringToDecodeTargetIndications(
    std::string_view symbols) {
  std::vector<DecodeTargetIndication> dtis;
  dtis.reserve(symbols.size());
  for (char symbol : symbols) {
    DecodeTargetIndication indication;
    switch (symbol) {
      case '-':
        indication = DecodeTargetIndication::kNotPresent;
        break;
      case 'D':
        indication = DecodeTargetIndication::kDiscardable;
        break;
      case 'R':
        indication = DecodeTargetIndication::kRequired;
        break;
      case 'S':
        indication = DecodeTargetIndication::kSwitch;
        break;
      default:
        assert(false);
    }
    dtis.push_back(indication);
  }
  return dtis;
}

}  // namespace webrtc_impl
}  // namespace webrtc

/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <brtc/video/generic_frame_info.h>

#include <utility>

namespace webrtc {

GenericFrameInfo::GenericFrameInfo() = default;
GenericFrameInfo::GenericFrameInfo(const GenericFrameInfo&) = default;
GenericFrameInfo::~GenericFrameInfo() = default;

GenericFrameInfo::Builder::Builder() = default;
GenericFrameInfo::Builder::~Builder() = default;

GenericFrameInfo GenericFrameInfo::Builder::Build() const {
  return info_;
}

GenericFrameInfo::Builder& GenericFrameInfo::Builder::T(int _temporal_id) {
  info_.temporal_id = _temporal_id;
  return *this;
}

GenericFrameInfo::Builder& GenericFrameInfo::Builder::S(int _spatial_id) {
  info_.spatial_id = _spatial_id;
  return *this;
}

GenericFrameInfo::Builder& GenericFrameInfo::Builder::Dtis(
    std::string_view indication_symbols) {
  info_.decode_target_indications =
      webrtc_impl::StringToDecodeTargetIndications(indication_symbols);
  return *this;
}

}  // namespace webrtc

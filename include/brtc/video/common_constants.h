/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains constants that are used by multiple global
// codec definitions (modules/video_coding/codecs/*/include/*_globals.h)

#pragma once

#include <stdint.h>

namespace webrtc {

const int16_t kNoPictureId = -1;
const int16_t kNoTl0PicIdx = -1;
const uint8_t kNoTemporalIdx = 0xFF;
const int kNoKeyIdx = -1;

const int kMaxEncoderBuffers = 8;
const int kMaxSimulcastStreams = 3;
const int kMaxSpatialLayers = 5;
const int kMaxTemporalStreams = 4;
const int kMaxPreferredPixelFormats = 5;

}  // namespace webrtc


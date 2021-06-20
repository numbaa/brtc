/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains codec dependent definitions that are needed in
// order to compile the WebRTC codebase, even if this codec is not used.

#pragma once

#include <string>

#include "common_constants.h"

namespace webrtc {

// The packetization types that we support: single, aggregated, and fragmented.
enum class H264PacketizationTypes {
  kH264SingleNalu,  // This packet contains a single NAL unit.
  kH264StapA,       // This packet contains STAP-A (single time
                    // aggregation) packets. If this packet has an
                    // associated NAL unit type, it'll be for the
                    // first such aggregated packet.
  kH264FuA,         // This packet contains a FU-A (fragmentation
                    // unit) packet, meaning it is a part of a frame
                    // that was too large to fit into a single packet.
};

// Packetization modes are defined in RFC 6184 section 6
// Due to the structure containing this being initialized with zeroes
// in some places, and mode 1 being default, mode 1 needs to have the value
// zero. https://crbug.com/webrtc/6803
enum class H264PacketizationMode {
  NonInterleaved = 0,  // Mode 1 - STAP-A, FU-A is allowed
  SingleNalUnit        // Mode 0 - only single NALU allowed
};

// This function is declared inline because it is not clear which
// .cc file it should belong to.
// TODO(hta): Refactor. https://bugs.webrtc.org/6842
// TODO(jonasolsson): Use absl::string_view instead when that's available.
inline std::string ToString(H264PacketizationMode mode) {
  if (mode == H264PacketizationMode::NonInterleaved) {
    return "NonInterleaved";
  } else if (mode == H264PacketizationMode::SingleNalUnit) {
    return "SingleNalUnit";
  }
  assert(false);
  return "";
}

enum class H264NaluType : uint8_t {
    Slice = 1,
    Idr = 5,
    Sei = 6,
    Sps = 7,
    Pps = 8,
    Aud = 9,
    EndOfSequence = 10,
    EndOfStream = 11,
    Filler = 12,
    StapA = 24,
    FuA = 28
};

struct NaluInfo {
    H264NaluType type;
    int sps_id;
    int pps_id;
};

const size_t kMaxNalusPerPacket = 10;

struct RTPVideoHeaderH264 {
  // The NAL unit type. If this is a header for a
  // fragmented packet, it's the NAL unit type of
  // the original data. If this is the header for an
  // aggregated packet, it's the NAL unit type of
  // the first NAL unit in the packet.
  H264NaluType nalu_type;
  // The packetization type of this buffer - single, aggregated or fragmented.
  H264PacketizationTypes packetization_type;
  NaluInfo nalus[kMaxNalusPerPacket];
  size_t nalus_length;
  // The packetization mode of this transport. Packetization mode
  // determines which packetization types are allowed when packetizing.
  H264PacketizationMode packetization_mode;

  //uint16_t picture_id;
  //int64_t timestamp_local;
  //bool has_last_fragement;
};

}  // namespace webrtc


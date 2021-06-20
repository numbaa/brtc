/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video/depacketizer/video_rtp_depacketizer_h264.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include <optional>
//#include "common_video/h264/h264_common.h"
#include "video/depacketizer/pps_parser.h"
#include "video/depacketizer/sps_parser.h"
//#include "common_video/h264/sps_vui_rewriter.h"
#include <bco/utils.h>

#include <glog/logging.h>

namespace webrtc {
namespace {
constexpr size_t kNaluTypeSize = 1;
constexpr size_t kNalHeaderSize = 1;
constexpr size_t kFuAHeaderSize = 2;
constexpr size_t kLengthFieldSize = 2;
constexpr size_t kStapAHeaderSize = kNalHeaderSize + kLengthFieldSize;

// Bit masks for FU (A and B) indicators.
enum NalDefs : uint8_t { kFBit = 0x80, kNriMask = 0x60, kTypeMask = 0x1F };

// Bit masks for FU (A and B) headers.
enum FuDefs : uint8_t { kSBit = 0x80, kEBit = 0x40, kRBit = 0x20 };

// TODO(pbos): Avoid parsing this here as well as inside the jitter buffer.
bool ParseStapAStartOffsets(const uint8_t* nalu_ptr,
                            size_t length_remaining,
                            std::vector<size_t>* offsets) {
  size_t offset = 0;
  while (length_remaining > 0) {
    // Buffer doesn't contain room for additional nalu length.
    if (length_remaining < sizeof(uint16_t))
      return false;
    uint16_t nalu_size;
    bco::read_big_endian(nalu_ptr, nalu_size);
    nalu_ptr += sizeof(uint16_t);
    length_remaining -= sizeof(uint16_t);
    if (nalu_size > length_remaining)
      return false;
    nalu_ptr += nalu_size;
    length_remaining -= nalu_size;

    offsets->push_back(offset + kStapAHeaderSize);
    offset += kLengthFieldSize + nalu_size;
  }
  return true;
}

void ProcessStapAOrSingleNalu(brtc::RtpPacket& packet) {
  const auto data = packet.data();
  auto& video_header = packet.video_header();
  bool modified_buffer = false;
  video_header.width = 0;
  video_header.height = 0;
  video_header.codec = brtc::VideoCodecType::H264;
  video_header.simulcastIdx = 0;
  video_header.is_first_packet_in_frame = true;
  auto& h264_header = video_header.video_type_header.emplace<brtc::RTPVideoHeaderH264>();

  const uint8_t* nalu_start = payload_data + kNalHeaderSize;
  const size_t nalu_length = rtp_payload.size() - kNalHeaderSize;
  H264NaluType nal_type = static_cast<H264NaluType>(payload_data[0] & kTypeMask);
  std::vector<size_t> nalu_start_offsets;
  if (nal_type == H264NaluType::StapA) {
    // Skip the StapA header (StapA NAL type + length).
    if (rtp_payload.size() <= kStapAHeaderSize) {
      LOG(ERROR) << "StapA header truncated.";
      return;
    }

    if (!ParseStapAStartOffsets(nalu_start, nalu_length, &nalu_start_offsets)) {
      LOG(ERROR) << "StapA packet with incorrect NALU packet lengths.";
      return;
    }

    h264_header.packetization_type = H264PacketizationTypes::kH264StapA;
    nal_type = payload_data[kStapAHeaderSize] & kTypeMask;
  } else {
    h264_header.packetization_type = H264PacketizationTypes::kH264SingleNalu;
    nalu_start_offsets.push_back(0);
  }
  h264_header.nalu_type = nal_type;
  packet.video_header().frame_type = brtc::VideoFrameType::VideoFrameDelta;

  nalu_start_offsets.push_back(rtp_payload.size() +
                               kLengthFieldSize);  // End offset.
  for (size_t i = 0; i < nalu_start_offsets.size() - 1; ++i) {
    size_t start_offset = nalu_start_offsets[i];
    // End offset is actually start offset for next unit, excluding length field
    // so remove that from this units length.
    size_t end_offset = nalu_start_offsets[i + 1] - kLengthFieldSize;
    if (end_offset - start_offset < kNaluTypeSize) {
      LOG(ERROR) << "STAP-A packet too short";
      return;
    }

    NaluInfo nalu;
    nalu.type = payload_data[start_offset] & kTypeMask;
    nalu.sps_id = -1;
    nalu.pps_id = -1;
    start_offset += kNaluTypeSize;

    switch (nalu.type) {
      case H264NaluType::Sps: {
        // Check if VUI is present in SPS and if it needs to be modified to
        // avoid
        // excessive decoder latency.

        // Copy any previous data first (likely just the first header).
        rtc::Buffer output_buffer;
        if (start_offset)
          output_buffer.AppendData(payload_data, start_offset);

        std::optional<SpsParser::SpsState> sps;

        SpsVuiRewriter::ParseResult result = SpsVuiRewriter::ParseAndRewriteSps(
            &payload_data[start_offset], end_offset - start_offset, &sps,
            nullptr, &output_buffer, SpsVuiRewriter::Direction::kIncoming);

        if (result == SpsVuiRewriter::ParseResult::kVuiRewritten) {
          if (modified_buffer) {
            LOG(WARNING)
                << "More than one H264 SPS NAL units needing "
                   "rewriting found within a single STAP-A packet. "
                   "Keeping the first and rewriting the last.";
          }

          // Rewrite length field to new SPS size.
          if (h264_header.packetization_type == H264PacketizationTypes::kH264StapA) {
            size_t length_field_offset =
                start_offset - (kNaluTypeSize + kLengthFieldSize);
            // Stap-A Length includes payload data and type header.
            size_t rewritten_size =
                output_buffer.size() - start_offset + kNaluTypeSize;
            ByteWriter<uint16_t>::WriteBigEndian(
                &output_buffer[length_field_offset], rewritten_size);
          }

          packet.video_header().SetData(output_buffer.data(),
                                                output_buffer.size());
          // Append rest of packet.
          parsed_payload->video_payload.AppendData(
              &payload_data[end_offset],
              nalu_length + kNalHeaderSize - end_offset);

          modified_buffer = true;
        }

        if (sps) {
          packet.video_header().width = sps->width;
          packet.video_header().height = sps->height;
          nalu.sps_id = sps->id;
        } else {
          LOG(WARNING) << "Failed to parse SPS id from SPS slice.";
        }
        packet.video_header().frame_type = brtc::VideoFrameType::VideoFrameKey;
        break;
      }
      case H264NaluType::Pps: {
        uint32_t pps_id;
        uint32_t sps_id;
        if (PpsParser::ParsePpsIds(&payload_data[start_offset],
                                   end_offset - start_offset, &pps_id,
                                   &sps_id)) {
          nalu.pps_id = pps_id;
          nalu.sps_id = sps_id;
        } else {
          LOG(WARNING)
              << "Failed to parse PPS id and SPS id from PPS slice.";
        }
        break;
      }
      case H264NaluType::Idr:
        packet.video_header().frame_type = brtc::VideoFrameType::VideoFrameKey;
        [[fallthrough]];
      case H264NaluType::Slice: {
        std::optional<uint32_t> pps_id = PpsParser::ParsePpsIdFromSlice(
            &payload_data[start_offset], end_offset - start_offset);
        if (pps_id) {
          nalu.pps_id = *pps_id;
        } else {
          LOG(WARNING) << "Failed to parse PPS id from slice of type: "
                              << static_cast<int>(nalu.type);
        }
        break;
      }
      // Slices below don't contain SPS or PPS ids.
      case H264NaluType::Aud:
      case H264NaluType::EndOfSequence:
      case H264NaluType::EndOfStream:
      case H264NaluType::Filler:
      case H264NaluType::Sei:
        break;
      case H264NaluType::StapA:
      case H264NaluType::FuA:
        LOG(WARNING) << "Unexpected STAP-A or FU-A received.";
        return;
    }

    if (h264_header.nalus_length == kMaxNalusPerPacket) {
      LOG(WARNING)
          << "Received packet containing more than " << kMaxNalusPerPacket
          << " NAL units. Will not keep track sps and pps ids for all of them.";
    } else {
      h264_header.nalus[h264_header.nalus_length++] = nalu;
    }
  }

  return parsed_payload;
}

/*
absl::optional<VideoRtpDepacketizer::ParsedRtpPayload> ParseFuaNalu(
    rtc::CopyOnWriteBuffer rtp_payload) {
  if (rtp_payload.size() < kFuAHeaderSize) {
    LOG(ERROR) << "FU-A NAL units truncated.";
    return absl::nullopt;
  }
  absl::optional<VideoRtpDepacketizer::ParsedRtpPayload> parsed_payload(
      absl::in_place);
  uint8_t fnri = rtp_payload.cdata()[0] & (kFBit | kNriMask);
  uint8_t original_nal_type = rtp_payload.cdata()[1] & kTypeMask;
  bool first_fragment = (rtp_payload.cdata()[1] & kSBit) > 0;
  NaluInfo nalu;
  nalu.type = original_nal_type;
  nalu.sps_id = -1;
  nalu.pps_id = -1;
  if (first_fragment) {
    absl::optional<uint32_t> pps_id =
        PpsParser::ParsePpsIdFromSlice(rtp_payload.cdata() + 2 * kNalHeaderSize,
                                       rtp_payload.size() - 2 * kNalHeaderSize);
    if (pps_id) {
      nalu.pps_id = *pps_id;
    } else {
      LOG(LS_WARNING)
          << "Failed to parse PPS from first fragment of FU-A NAL "
             "unit with original type: "
          << static_cast<int>(nalu.type);
    }
    uint8_t original_nal_header = fnri | original_nal_type;
    rtp_payload =
        rtp_payload.Slice(kNalHeaderSize, rtp_payload.size() - kNalHeaderSize);
    rtp_payload.MutableData()[0] = original_nal_header;
    parsed_payload->video_payload = std::move(rtp_payload);
  } else {
    parsed_payload->video_payload =
        rtp_payload.Slice(kFuAHeaderSize, rtp_payload.size() - kFuAHeaderSize);
  }

  if (original_nal_type == H264::NaluType::kIdr) {
    parsed_payload->video_header.frame_type = VideoFrameType::kVideoFrameKey;
  } else {
    parsed_payload->video_header.frame_type = VideoFrameType::kVideoFrameDelta;
  }
  parsed_payload->video_header.width = 0;
  parsed_payload->video_header.height = 0;
  parsed_payload->video_header.codec = kVideoCodecH264;
  parsed_payload->video_header.simulcastIdx = 0;
  parsed_payload->video_header.is_first_packet_in_frame = first_fragment;
  auto& h264_header = parsed_payload->video_header.video_type_header
                          .emplace<RTPVideoHeaderH264>();
  h264_header.packetization_type = kH264FuA;
  h264_header.nalu_type = original_nal_type;
  if (first_fragment) {
    h264_header.nalus[h264_header.nalus_length] = nalu;
    h264_header.nalus_length = 1;
  }
  return parsed_payload;
}
*/

}  // namespace

void VideoRtpDepacketizerH264::Parse(brtc::RtpPacket& packet) {
  if (packet.size() == 0) {
    LOG(ERROR) << "Empty payload.";
    return;
  }

  uint8_t nal_type = packet.data()[0] & kTypeMask;

  if (nal_type == H264NaluType::FuA) {
    // Fragmented NAL units (FU-A).
    ParseFuaNalu(packet);
  } else {
    // We handle STAP-A and single NALU's the same way here. The jitter buffer
    // will depacketize the STAP-A into NAL units later.
    ProcessStapAOrSingleNalu(packet);
  }
}

}  // namespace webrtc

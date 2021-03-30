/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <glog/logging.h>
#include "../common/time_utils.h"
#include "frame_assembler.h"

namespace brtc {

void FrameAssembler::insert(RtpPacket packet)
{
    uint16_t seq_num = packet.sequence_number();
    size_t index = seq_num % buffer_.size();

    if (!first_packet_received_) {
        first_seq_num_ = seq_num;
        first_packet_received_ = true;
    } else if (webrtc::AheadOf(first_seq_num_, seq_num)) {
        // If we have explicitly cleared past this packet then it's old,
        // don't insert it, just silently ignore it.
        if (is_cleared_to_first_seq_num_) {
            return;
        }

        first_seq_num_ = seq_num;
    }

    if (buffer_[index].size() != 0) {
        // Duplicate packet, just delete the payload.
        if (buffer_[index].sequence_number() == packet.sequence_number()) {
            return;
        }

        // The packet buffer is full, try to expand the buffer.
        while (expand_buffer() && buffer_[seq_num % buffer_.size()].size() != 0) {
        }
        index = seq_num % buffer_.size();

        // Packet buffer is still full since we were unable to expand the buffer.
        if (buffer_[index].size() != 0) {
            // Clear the buffer, delete payload, and return false to signal that a
            // new keyframe is needed.
            LOG(WARNING) << "Clear PacketBuffer and request key frame.";
            clear_internal();
            //可能我也要某种形式通知buffer满了
            //result.buffer_cleared = true;
            return;
        }
    }

    int64_t now_ms = MachineNowMilliseconds();
    last_received_packet_ms_ = now_ms;
    if (packet->video_header.frame_type == VideoFrameType::kVideoFrameKey || last_received_keyframe_rtp_timestamp_ == packet.timestamp()) {
        last_received_keyframe_packet_ms_ = now_ms;
        last_received_keyframe_rtp_timestamp_ = packet.timestamp();
    }

    packet->continuous = false;
    buffer_[index] = std::move(packet);

    update_missing_packets(seq_num);

    //result.packets = find_frames(seq_num);
    //return result;
}

std::vector<RtpPacket> FrameAssembler::pop_assembled_frame()
{
    return {};
}

void FrameAssembler::update_missing_packets(uint16_t seq_num)
{
    if (!newest_inserted_seq_num_)
        newest_inserted_seq_num_ = seq_num;

    const int kMaxPaddingAge = 1000;
    if (webrtc::AheadOf(seq_num, *newest_inserted_seq_num_)) {
        uint16_t old_seq_num = seq_num - kMaxPaddingAge;
        auto erase_to = missing_packets_.lower_bound(old_seq_num);
        missing_packets_.erase(missing_packets_.begin(), erase_to);

        // Guard against inserting a large amount of missing packets if there is a
        // jump in the sequence number.
        if (webrtc::AheadOf(old_seq_num, *newest_inserted_seq_num_))
            *newest_inserted_seq_num_ = old_seq_num;

        ++*newest_inserted_seq_num_;
        while (webrtc::AheadOf(seq_num, *newest_inserted_seq_num_)) {
            missing_packets_.insert(*newest_inserted_seq_num_);
            ++*newest_inserted_seq_num_;
        }
    } else {
        missing_packets_.erase(seq_num);
    }
}

bool FrameAssembler::potential_new_frame(uint16_t seq_num) const
{
    size_t index = seq_num % buffer_.size();
    int prev_index = index > 0 ? index - 1 : buffer_.size() - 1;
    const auto& entry = buffer_[index];
    const auto& prev_entry = buffer_[prev_index];

    if (entry.size() == 0)
        return false;
    if (entry.sequence_number() != seq_num)
        return false;
    if (entry->is_first_packet_in_frame())
        return true;
    if (prev_entry.size() == 0)
        return false;
    if (prev_entry.sequence_number() != static_cast<uint16_t>(entry.sequence_number() - 1))
        return false;
    if (prev_entry.timestamp() != entry.timestamp())
        return false;
    if (prev_entry->continuous)
        return true;

    return false;
}

std::vector<RtpPacket> FrameAssembler::find_frames(uint16_t seq_num)
{
    std::vector<RtpPacket> found_frames;
    for (size_t i = 0; i < buffer_.size() && potential_new_frame(seq_num); ++i) {
        size_t index = seq_num % buffer_.size();
        buffer_[index]->continuous = true;

        // If all packets of the frame is continuous, find the first packet of the
        // frame and add all packets of the frame to the returned packets.
        if (buffer_[index]->is_last_packet_in_frame()) {
            uint16_t start_seq_num = seq_num;

            // Find the start index by searching backward until the packet with
            // the |frame_begin| flag is set.
            int start_index = index;
            size_t tested_packets = 0;
            int64_t frame_timestamp = buffer_[start_index]->timestamp;

            // Identify H.264 keyframes by means of SPS, PPS, and IDR.
            bool is_h264 = buffer_[start_index]->codec() == kVideoCodecH264;
            bool has_h264_sps = false;
            bool has_h264_pps = false;
            bool has_h264_idr = false;
            bool is_h264_keyframe = false;
            int idr_width = -1;
            int idr_height = -1;
            while (true) {
                ++tested_packets;

                if (!is_h264 && buffer_[start_index]->is_first_packet_in_frame())
                    break;

                if (is_h264) {
                    const auto* h264_header = absl::get_if<RTPVideoHeaderH264>(
                        &buffer_[start_index]->video_header.video_type_header);
                    if (!h264_header || h264_header->nalus_length >= kMaxNalusPerPacket)
                        return found_frames;

                    for (size_t j = 0; j < h264_header->nalus_length; ++j) {
                        if (h264_header->nalus[j].type == H264::NaluType::kSps) {
                            has_h264_sps = true;
                        } else if (h264_header->nalus[j].type == H264::NaluType::kPps) {
                            has_h264_pps = true;
                        } else if (h264_header->nalus[j].type == H264::NaluType::kIdr) {
                            has_h264_idr = true;
                        }
                    }
                    if ((sps_pps_idr_is_h264_keyframe_ && has_h264_idr && has_h264_sps && has_h264_pps) || (!sps_pps_idr_is_h264_keyframe_ && has_h264_idr)) {
                        is_h264_keyframe = true;
                        // Store the resolution of key frame which is the packet with
                        // smallest index and valid resolution; typically its IDR or SPS
                        // packet; there may be packet preceeding this packet, IDR's
                        // resolution will be applied to them.
                        if (buffer_[start_index]->width() > 0 && buffer_[start_index]->height() > 0) {
                            idr_width = buffer_[start_index]->width();
                            idr_height = buffer_[start_index]->height();
                        }
                    }
                }

                if (tested_packets == buffer_.size())
                    break;

                start_index = start_index > 0 ? start_index - 1 : buffer_.size() - 1;

                // In the case of H264 we don't have a frame_begin bit (yes,
                // |frame_begin| might be set to true but that is a lie). So instead
                // we traverese backwards as long as we have a previous packet and
                // the timestamp of that packet is the same as this one. This may cause
                // the PacketBuffer to hand out incomplete frames.
                // See: https://bugs.chromium.org/p/webrtc/issues/detail?id=7106
                if (is_h264 && (buffer_[start_index].size() == 0 || buffer_[start_index].timestamp() != frame_timestamp)) {
                    break;
                }

                --start_seq_num;
            }

            if (is_h264) {
                // Warn if this is an unsafe frame.
                if (has_h264_idr && (!has_h264_sps || !has_h264_pps)) {
                    LOG(WARNING)
                        << "Received H.264-IDR frame "
                           "(SPS: "
                        << has_h264_sps << ", PPS: " << has_h264_pps << "). Treating as "
                        << (sps_pps_idr_is_h264_keyframe_ ? "delta" : "key")
                        << " frame since WebRTC-SpsPpsIdrIsH264Keyframe is "
                        << (sps_pps_idr_is_h264_keyframe_ ? "enabled." : "disabled");
                }

                // Now that we have decided whether to treat this frame as a key frame
                // or delta frame in the frame buffer, we update the field that
                // determines if the RtpFrameObject is a key frame or delta frame.
                const size_t first_packet_index = start_seq_num % buffer_.size();
                if (is_h264_keyframe) {
                    buffer_[first_packet_index]->video_header.frame_type = VideoFrameType::kVideoFrameKey;
                    if (idr_width > 0 && idr_height > 0) {
                        // IDR frame was finalized and we have the correct resolution for
                        // IDR; update first packet to have same resolution as IDR.
                        buffer_[first_packet_index]->video_header.width = idr_width;
                        buffer_[first_packet_index]->video_header.height = idr_height;
                    }
                } else {
                    buffer_[first_packet_index]->video_header.frame_type = VideoFrameType::kVideoFrameDelta;
                }

                // If this is not a keyframe, make sure there are no gaps in the packet
                // sequence numbers up until this point.
                if (!is_h264_keyframe && missing_packets_.upper_bound(start_seq_num) != missing_packets_.begin()) {
                    return found_frames;
                }
            }

            const uint16_t end_seq_num = seq_num + 1;
            // Use uint16_t type to handle sequence number wrap around case.
            uint16_t num_packets = end_seq_num - start_seq_num;
            found_frames.reserve(found_frames.size() + num_packets);
            for (uint16_t i = start_seq_num; i != end_seq_num; ++i) {
                RtpPacket& packet = buffer_[i % buffer_.size()];
                assert(i == packet.sequence_number());
                // Ensure frame boundary flags are properly set.
                packet->video_header.is_first_packet_in_frame = (i == start_seq_num);
                packet->video_header.is_last_packet_in_frame = (i == seq_num);
                found_frames.push_back(std::move(packet));
            }

            missing_packets_.erase(missing_packets_.begin(),
                missing_packets_.upper_bound(seq_num));
        }
        ++seq_num;
    }
    return found_frames;
}

bool FrameAssembler::expand_buffer()
{
    if (buffer_.size() == max_size_) {
        LOG(WARNING) << "PacketBuffer is already at max size (" << max_size_
                            << "), failed to increase size.";
        return false;
    }

    size_t new_size = std::min(max_size_, 2 * buffer_.size());
    std::vector<RtpPacket> new_buffer(new_size);
    for (auto& entry : buffer_) {
        if (entry.size() != 0) {
            new_buffer[entry.sequence_number() % new_size] = entry;
        }
    }
    buffer_ = std::move(new_buffer);
    LOG(INFO) << "PacketBuffer size expanded to " << new_size;
    return true;
}

void FrameAssembler::clear_internal()
{
    for (auto& entry : buffer_) {
        entry = RtpPacket {};
    }

    first_packet_received_ = false;
    is_cleared_to_first_seq_num_ = false;
    last_received_packet_ms_.reset();
    last_received_keyframe_packet_ms_.reset();
    newest_inserted_seq_num_.reset();
    missing_packets_.clear();
}

} // namespace brtc

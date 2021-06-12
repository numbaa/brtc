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
#include "common/time_utils.h"
#include "video/frame_assembler/frame_assembler.h"

namespace brtc {

FrameAssembler::FrameAssembler(size_t start_size, size_t max_size)
    : max_size_(max_size)
    , buffer_(start_size)
{
}

void FrameAssembler::insert(RtpPacket rtp_packet)
{
    if (rtp_packet.video_header().is_first_packet_in_frame) {
        video_header_ = rtp_packet.video_header();
    }
    uint16_t seq_num = rtp_packet.sequence_number();
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

    if (not buffer_[index].empty_payload()) {
        // Duplicate packet, just delete the payload.
        if (buffer_[index].sequence_number() == rtp_packet.sequence_number()) {
            return;
        }

        // The packet buffer is full, try to expand the buffer.
        while (expand_buffer() && !buffer_[seq_num % buffer_.size()].empty_payload()) {
        }
        index = seq_num % buffer_.size();

        // Packet buffer is still full since we were unable to expand the buffer.
        if (not buffer_[index].empty_payload()) {
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
    auto& video_header = rtp_packet.video_header();
    last_received_packet_ms_ = now_ms;
    if (video_header.frame_type == VideoFrameType::VideoFrameKey || last_received_keyframe_rtp_timestamp_ == rtp_packet.timestamp()) {
        last_received_keyframe_packet_ms_ = now_ms;
        last_received_keyframe_rtp_timestamp_ = rtp_packet.timestamp();
    }

    Packet packet { rtp_packet };
    packet.continuous = false;
    buffer_[index] = packet;

    update_missing_packets(seq_num);

    find_frames(seq_num);
}

std::unique_ptr<ReceivedFrame> FrameAssembler::pop_assembled_frame()
{
    if (assembled_frames_.empty())
        return nullptr;
    auto& packets = assembled_frames_.front();
    auto frame_data = std::make_shared<std::vector<uint8_t>>();
    for (auto& packet : packets) {
        auto payload_spans = packet.payload().data();
        for (auto span : payload_spans) {
            std::ranges::copy(span, std::back_inserter(*frame_data));
        }
    }
    assembled_frames_.pop_front();
    auto frame = std::make_unique<ReceivedFrame>();
    frame->underlying_type = Frame::UnderlyingType::kMemory;
    frame->data = frame_data->data();
    frame->_data_holder = frame_data;

    frame->codec_type = VideoCodecType::H264;
    const RTPVideoHeaderH264& video_header = std::get<RTPVideoHeaderH264>(frame->video_header.video_type_header);
    //TODO: fill other fields
    //frame->frame_type
    if (video_header_.has_value()) {
        frame->video_header = video_header_.value();
        frame->id = frame->video_header.generic.value().frame_id;
        frame->frame_type = frame->video_header.frame_type;
    }
    return frame;
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
    size_t prev_index = index > 0 ? index - 1 : buffer_.size() - 1;
    const auto& entry = buffer_[index];
    const auto& prev_entry = buffer_[prev_index];

    if (entry.empty_payload())
        return false;
    if (entry.sequence_number() != seq_num)
        return false;
    if (entry.video_header().is_first_packet_in_frame)
        return true;
    if (prev_entry.empty_payload())
        return false;
    if (prev_entry.sequence_number() != static_cast<uint16_t>(entry.sequence_number() - 1))
        return false;
    if (prev_entry.timestamp() != entry.timestamp())
        return false;
    if (prev_entry.continuous)
        return true;

    return false;
}

void FrameAssembler::find_frames(uint16_t seq_num)
{
    //std::vector<RtpPacket> found_frames;
    for (size_t i = 0; i < buffer_.size() && potential_new_frame(seq_num); ++i) {
        size_t index = seq_num % buffer_.size();
        buffer_[index].continuous = true;

        // If all packets of the frame is continuous, find the first packet of the
        // frame and add all packets of the frame to the returned packets.
        if (buffer_[index].video_header().is_last_packet_in_frame) {
            uint16_t start_seq_num = seq_num;

            // Find the start index by searching backward until the packet with
            // the |frame_begin| flag is set.
            size_t start_index = index;
            size_t tested_packets = 0;
            int64_t frame_timestamp = buffer_[start_index].sequence_number();

            // Identify H.264 keyframes by means of SPS, PPS, and IDR.
            bool is_h264 = buffer_[start_index].video_header().codec == VideoCodecType::H264;
            bool has_h264_sps = false;
            bool has_h264_pps = false;
            bool has_h264_idr = false;
            bool is_h264_keyframe = false;
            int idr_width = -1;
            int idr_height = -1;
            while (true) {
                ++tested_packets;

                if (!is_h264 && buffer_[start_index].video_header().is_first_packet_in_frame)
                    break;

                if (is_h264) {
                    const auto& h264_header = std::get<RTPVideoHeaderH264>(buffer_[start_index].video_header().video_type_header);
                    //if (!h264_header || h264_header.nalus_length >= kMaxNalusPerPacket)
                    if (h264_header.nalus_length >= kMaxNalusPerPacket)
                        return; //return found_frames;

                    for (size_t j = 0; j < h264_header.nalus_length; ++j) {
                        if (h264_header.nalus[j].type == static_cast<uint8_t>(H264NaluType::Sps)) {
                            has_h264_sps = true;
                        } else if (h264_header.nalus[j].type == static_cast<uint8_t>(H264NaluType::Pps)) {
                            has_h264_pps = true;
                        } else if (h264_header.nalus[j].type == static_cast<uint8_t>(H264NaluType::Idr)) {
                            has_h264_idr = true;
                        }
                    }
                    if ((sps_pps_idr_is_h264_keyframe_ && has_h264_idr && has_h264_sps && has_h264_pps) || (!sps_pps_idr_is_h264_keyframe_ && has_h264_idr)) {
                        is_h264_keyframe = true;
                        // Store the resolution of key frame which is the packet with
                        // smallest index and valid resolution; typically its IDR or SPS
                        // packet; there may be packet preceeding this packet, IDR's
                        // resolution will be applied to them.
                        if (buffer_[start_index].video_header().width > 0 && buffer_[start_index].video_header().height > 0) {
                            idr_width = buffer_[start_index].video_header().width;
                            idr_height = buffer_[start_index].video_header().height;
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
                if (is_h264 && (buffer_[start_index].empty_payload() || buffer_[start_index].timestamp() != frame_timestamp)) {
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
                    buffer_[first_packet_index].video_header().frame_type = VideoFrameType::VideoFrameKey;
                    if (idr_width > 0 && idr_height > 0) {
                        // IDR frame was finalized and we have the correct resolution for
                        // IDR; update first packet to have same resolution as IDR.
                        buffer_[first_packet_index].video_header().width = idr_width;
                        buffer_[first_packet_index].video_header().height = idr_height;
                    }
                } else {
                    buffer_[first_packet_index].video_header().frame_type = VideoFrameType::VideoFrameDelta;
                }

                // If this is not a keyframe, make sure there are no gaps in the packet
                // sequence numbers up until this point.
                if (!is_h264_keyframe && missing_packets_.upper_bound(start_seq_num) != missing_packets_.begin()) {
                    return; //return found_frames;
                }
            }

            const uint16_t end_seq_num = seq_num + 1;
            // Use uint16_t type to handle sequence number wrap around case.
            uint16_t num_packets = end_seq_num - start_seq_num;
            std::vector<RtpPacket> found_frames;
            found_frames.reserve(found_frames.size() + num_packets);
            for (uint16_t i = start_seq_num; i != end_seq_num; ++i) {
                RtpPacket& packet = buffer_[i % buffer_.size()];
                assert(i == packet.sequence_number());
                // Ensure frame boundary flags are properly set.
                packet.video_header().is_first_packet_in_frame = (i == start_seq_num);
                packet.video_header().is_last_packet_in_frame = (i == seq_num);
                found_frames.push_back(packet);
            }
            if (not found_frames.empty()) {
                assembled_frames_.push_back(found_frames);
            }

            missing_packets_.erase(missing_packets_.begin(),
                missing_packets_.upper_bound(seq_num));
        }
        ++seq_num;
    }
}

bool FrameAssembler::expand_buffer()
{
    if (buffer_.size() == max_size_) {
        LOG(WARNING) << "PacketBuffer is already at max size (" << max_size_
                            << "), failed to increase size.";
        return false;
    }

    size_t new_size = std::min(max_size_, 2 * buffer_.size());
    std::vector<Packet> new_buffer(new_size);
    for (auto& entry : buffer_) {
        if (not entry.empty_payload()) {
            new_buffer[entry.sequence_number() % new_size] = entry;
        }
    }
    buffer_ = new_buffer;
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

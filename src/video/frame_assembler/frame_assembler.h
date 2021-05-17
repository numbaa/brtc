#pragma once
#include <optional>
#include <set>
#include <deque>
#include <brtc/frame.h>
#include "common/sequence_number_util.h"
#include "rtp/rtp.h"

namespace brtc {

class FrameAssembler {
private:
    class Packet : public RtpPacket {
    public:
        Packet() = default;
        Packet(RtpPacket&& rtp_packet)
            : RtpPacket { std::move(rtp_packet) }
        {
        }
        bool continuous = false;
    };

public:
    FrameAssembler(size_t start_size, size_t max_size);
    void insert(RtpPacket packet);
    std::unique_ptr<ReceivedFrame> pop_assembled_frame();

private:
    void update_missing_packets(uint16_t seq_num);
    bool potential_new_frame(uint16_t seq_num) const;
    void find_frames(uint16_t seq_num);
    bool expand_buffer();
    void clear_internal();


private:
    bool first_packet_received_ = false;
    uint16_t first_seq_num_ = 0;
    bool is_cleared_to_first_seq_num_ = false;
    bool sps_pps_idr_is_h264_keyframe_ = false;
    std::optional<uint16_t> newest_inserted_seq_num_;
    std::set<uint16_t, webrtc::DescendingSeqNumComp<uint16_t>> missing_packets_;
    std::optional<int64_t> last_received_packet_ms_;
    std::optional<uint32_t> last_received_keyframe_rtp_timestamp_;
    std::optional<int64_t> last_received_keyframe_packet_ms_;
    std::vector<Packet> buffer_;
    std::deque<std::vector<RtpPacket>> assembled_frames_;
    const size_t max_size_; //构造函数传进来
};

}
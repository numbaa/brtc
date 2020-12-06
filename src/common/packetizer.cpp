#include <cassert>
#include "packetizer.h"

namespace brtc
{

static std::tuple<uint8_t*, int32_t> find_nalu(std::span<uint8_t> data)
{
    if (data.size() < kH264StartCodeLength) {
        return {nullptr, -1};
    }
    uint8_t* head = data.data();
    uint8_t* end = data.data() + data.size() - kH264StartCodeLength;
    while (head < end) {
        if (head[0]) {
            head++;
            continue;
        }
        if (head[1]) {
            head += 2;
            continue;
        }
        if (head[2]) {
            if (head[2] == 0x01) {
                return {head, 3};
            }
        }
        if (head[3] != 0x01) {
            head++;
            continue;
        }
        return {head, 4};
    }
    return {nullptr, -1};
}

Packetizer::Packetizer(std::span<uint8_t> encoded_frame)
    : frame_(encoded_frame)
{
    int32_t index = 0;
    uint8_t* nalus[kMaxNalusPerPacket+1];
    uint32_t start_code_lens[kMaxNalusPerPacket+1];
    std::span<uint8_t> remain_data = encoded_frame;
    while (index < kMaxNalusPerPacket) {
        auto [nalu, start_code_len] = find_nalu(remain_data);
        if (nalu == nullptr) {
            break;
        }
        nalus[index] = nalu;
        start_code_lens[index] = start_code_len;
        index++;
        remain_data = remain_data.subspan(start_code_len);
    }
    if (index == 0) {
        is_valid_frame_ = false;
        return;
    }
    nalus_len_ = index;
    for (int i=0; i<nalus_len_; i++) {
        nalus_[i] = StartCodeInfo{start_code_lens[i], nalus[i]-encoded_frame.data()};
    }
}

bool Packetizer::is_valid_frame() const
{
    return is_valid_frame_;
}

bool Packetizer::next_packet(RtpPacket& packet)
{
    assert(current_nalu_ < nalus_len_);
    packet.frame_ = frame_;
    packet.offset_of_frame_ = nalus_[current_nalu_].offset;
    current_nalu_++;
    return current_nalu_ < nalus_len_;
}

} // namespace brtc

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <queue>
#include "common/rtp.h"

namespace brtc {

class RtpFrameReferenceFinderImpl;
class OnCompleteFrameCallback;

class RtpFrameReferenceFinder {
public:
    using ReturnVector = std::vector<std::unique_ptr<ReceivedFrame>>;

    explicit RtpFrameReferenceFinder();
    explicit RtpFrameReferenceFinder(int64_t picture_id_offset);
    ~RtpFrameReferenceFinder();

    // Manage this frame until:
    //  - We have all information needed to determine its references, after
    //    which |frame_callback_| is called with the completed frame, or
    //  - We have too many stashed frames (determined by |kMaxStashedFrames|)
    //    so we drop this frame, or
    //  - It gets cleared by ClearTo, which also means we drop it.
    void ManageFrame(std::unique_ptr<ReceivedFrame> frame);

    // Notifies that padding has been received, which the reference finder
    // might need to calculate the references of a frame.
    ReturnVector PaddingReceived(uint16_t seq_num);

    // Clear all stashed frames that include packets older than |seq_num|.
    void ClearTo(uint16_t seq_num);

    std::unique_ptr<ReceivedFrame> pop_gop_inter_continous_frame();

private:

    // How far frames have been cleared out of the buffer by RTP sequence number.
    // A frame will be cleared if it contains a packet with a sequence number
    // older than |cleared_to_seq_num_|.
    int cleared_to_seq_num_ = -1;
    const int64_t picture_id_offset_;
    std::unique_ptr<RtpFrameReferenceFinderImpl> impl_;
    std::queue<std::unique_ptr<ReceivedFrame>> frames_;
};

} // namespace brtc

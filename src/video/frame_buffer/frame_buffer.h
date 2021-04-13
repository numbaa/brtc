#pragma once
#include <optional>
#include <map>
#include "common/rtp.h"
#include "video/frame_buffer/decoded_frames_history.h"

namespace brtc {

class FrameBuffer {
    struct FrameInfo {
        FrameInfo() = default;
        FrameInfo(FrameInfo&&) = default;
        ~FrameInfo() = default;

        // Which other frames that have direct unfulfilled dependencies
        // on this frame.
        std::vector<int64_t> dependent_frames;

        // A frame is continiuous if it has all its referenced/indirectly
        // referenced frames.
        //
        // How many unfulfilled frames this frame have until it becomes continuous.
        size_t num_missing_continuous = 0;

        // A frame is decodable if all its referenced frames have been decoded.
        //
        // How many unfulfilled frames this frame have until it becomes decodable.
        size_t num_missing_decodable = 0;

        // If this frame is continuous or not.
        bool continuous = false;

        // The actual EncodedFrame.
        //std::unique_ptr<ReceivedFrame> frame;
        std::optional<ReceivedFrame> frame;
    };
    using FrameMap = std::map<int64_t, FrameInfo>;

public:
    FrameBuffer(size_t decoded_history_size);
    void insert(ReceivedFrame frame);
    std::optional<ReceivedFrame> pop_decodable_frame();

private:
    bool valid_references(ReceivedFrame frame);
    void clear_frames_and_history();
    bool update_frame_info_with_incoming_frame(const ReceivedFrame& frame,
        FrameMap::iterator info);

private:
    std::optional<int64_t> last_continuous_frame_;
    FrameMap frames_;
    webrtc::video_coding::DecodedFramesHistory decoded_frames_history_;
    std::vector<FrameMap::iterator> frames_to_decode_;
    int64_t last_log_non_decoded_ms_;
};

} // namespace brtc
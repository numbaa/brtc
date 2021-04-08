#include <glog/logging.h>
#include "common/sequence_number_util.h"
#include "common/time_utils.h"
#include "video/frame_buffer/frame_buffer.h"

namespace {
// Max number of frames the buffer will hold.
constexpr size_t kMaxFramesBuffered = 800;

// Max number of decoded frame info that will be saved.
constexpr int kMaxFramesHistory = 1 << 13;

// The time it's allowed for a frame to be late to its rendering prediction and
// still be rendered.
constexpr int kMaxAllowedFrameDelayMs = 5;

constexpr int64_t kLogNonDecodedIntervalMs = 5000;
} // namespace

namespace brtc {

void FrameBuffer::insert(ReceivedFrame frame)
{
    //MutexLock lock(&mutex_);

    int64_t last_continuous_frame_id = last_continuous_frame_.value_or(-1);

    if (!valid_references(frame)) {
        LOG(WARNING) << "Frame " << frame.id
                            << " has invalid frame references, dropping frame.";
        //return last_continuous_frame_id;
        return;
    }

    if (frames_.size() >= kMaxFramesBuffered) {
        if (frame.frame_type == VideoFrameType::VideoFrameKey) {
            LOG(WARNING) << "Inserting keyframe " << frame.id
                                << " but buffer is full, clearing"
                                   " buffer and inserting the frame.";
            clear_frames_and_history();
        } else {
            LOG(WARNING) << "Frame " << frame.id
                                << " could not be inserted due to the frame "
                                   "buffer being full, dropping frame.";
            //return last_continuous_frame_id;
            return;
        }
    }

    auto last_decoded_frame = decoded_frames_history_.GetLastDecodedFrameId();
    auto last_decoded_frame_timestamp = decoded_frames_history_.GetLastDecodedFrameTimestamp();
    if (last_decoded_frame && frame.id <= *last_decoded_frame) {
        if (webrtc::AheadOf(frame.timestamp, *last_decoded_frame_timestamp) && frame.frame_type == VideoFrameType::VideoFrameKey) {
            // If this frame has a newer timestamp but an earlier frame id then we
            // assume there has been a jump in the frame id due to some encoder
            // reconfiguration or some other reason. Even though this is not according
            // to spec we can still continue to decode from this frame if it is a
            // keyframe.
            LOG(WARNING)
                << "A jump in frame id was detected, clearing buffer.";
            clear_frames_and_history();
            last_continuous_frame_id = -1;
        } else {
            LOG(WARNING) << "Frame " << frame.id << " inserted after frame "
                                << *last_decoded_frame
                                << " was handed off for decoding, dropping frame.";
            //return last_continuous_frame_id;
            return;
        }
    }

    // Test if inserting this frame would cause the order of the frames to become
    // ambiguous (covering more than half the interval of 2^16). This can happen
    // when the frame id make large jumps mid stream.
    if (!frames_.empty() && frame.id < frames_.begin()->first && frames_.rbegin()->first < frame.id) {
        LOG(WARNING) << "A jump in frame id was detected, clearing buffer.";
        clear_frames_and_history();
        last_continuous_frame_id = -1;
    }

    auto info = frames_.emplace(frame.id, FrameInfo()).first;

    if (info->second.frame) {
        //return last_continuous_frame_id;
        return;
    }

    if (!update_frame_info_with_incoming_frame(frame, info)) {
        //return last_continuous_frame_id;
        return;
    }

    //if (!frame->delayed_by_retransmission())
    //    timing_->IncomingTimestamp(frame.timestamp, frame->ReceivedTime());


    info->second.frame = std::move(frame);
}

std::optional<ReceivedFrame> FrameBuffer::pop_decodable_frame()
{
    return std::optional<ReceivedFrame>();
}

bool FrameBuffer::valid_references(ReceivedFrame frame)
{
    for (size_t i = 0; i < frame.num_references; ++i) {
        if (frame.references[i] >= frame.id)
            return false;

        for (size_t j = i + 1; j < frame.num_references; ++j) {
            if (frame.references[i] == frame.references[j])
                return false;
        }
    }

    return true;
}

void FrameBuffer::clear_frames_and_history()
{
    frames_.clear();
    last_continuous_frame_.reset();
    frames_to_decode_.clear();
    decoded_frames_history_.Clear();
}

bool FrameBuffer::update_frame_info_with_incoming_frame(const ReceivedFrame& frame, FrameMap::iterator info)
{
    auto last_decoded_frame = decoded_frames_history_.GetLastDecodedFrameId();
    assert(!last_decoded_frame || *last_decoded_frame < info->first);

    // In this function we determine how many missing dependencies this |frame|
    // has to become continuous/decodable. If a frame that this |frame| depend
    // on has already been decoded then we can ignore that dependency since it has
    // already been fulfilled.
    //
    // For all other frames we will register a backwards reference to this |frame|
    // so that |num_missing_continuous| and |num_missing_decodable| can be
    // decremented as frames become continuous/are decoded.
    struct Dependency {
        int64_t frame_id;
        bool continuous;
    };
    std::vector<Dependency> not_yet_fulfilled_dependencies;

    // Find all dependencies that have not yet been fulfilled.
    for (size_t i = 0; i < frame.num_references; ++i) {
        // Does |frame| depend on a frame earlier than the last decoded one?
        if (last_decoded_frame && frame.references[i] <= *last_decoded_frame) {
            // Was that frame decoded? If not, this |frame| will never become
            // decodable.
            if (!decoded_frames_history_.WasDecoded(frame.references[i])) {
                int64_t now_ms = MachineNowMilliseconds();
                if (last_log_non_decoded_ms_ + kLogNonDecodedIntervalMs < now_ms) {
                    LOG(WARNING)
                        << "Frame " << frame.id
                        << " depends on a non-decoded frame more previous than the last "
                           "decoded frame, dropping frame.";
                    last_log_non_decoded_ms_ = now_ms;
                }
                return false;
            }
        } else {
            auto ref_info = frames_.find(frame.references[i]);
            bool ref_continuous = ref_info != frames_.end() && ref_info->second.continuous;
            not_yet_fulfilled_dependencies.push_back(
                { frame.references[i], ref_continuous });
        }
    }

    info->second.num_missing_continuous = not_yet_fulfilled_dependencies.size();
    info->second.num_missing_decodable = not_yet_fulfilled_dependencies.size();

    for (const Dependency& dep : not_yet_fulfilled_dependencies) {
        if (dep.continuous)
            --info->second.num_missing_continuous;

        frames_[dep.frame_id].dependent_frames.push_back(frame.id);
    }

    return true;
}

} // namespace brtc

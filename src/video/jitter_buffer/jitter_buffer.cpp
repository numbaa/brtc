#include <queue>
#include <glog/logging.h>
#include "common/sequence_number_util.h"
#include "common/time_utils.h"
#include "video/jitter_buffer/jitter_buffer.h"

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
JitterBuffer::JitterBuffer(size_t decoded_history_size)
    : decoded_frames_history_(decoded_history_size)
{
}
void JitterBuffer::insert(ReceivedFrame frame)
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
    if (info->second.num_missing_continuous == 0) {
        info->second.continuous = true;
        propagate_continuity(info);
    }
}

bco::Task<ReceivedFrame> JitterBuffer::pop_decodable_frame()
{
    return decodale_frames_.recv();
}

bool JitterBuffer::valid_references(ReceivedFrame frame)
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

void JitterBuffer::clear_frames_and_history()
{
    frames_.clear();
    last_continuous_frame_.reset();
    decoded_frames_history_.Clear();
}

bool JitterBuffer::update_frame_info_with_incoming_frame(const ReceivedFrame& frame, FrameMap::iterator info)
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

void JitterBuffer::propagate_continuity(FrameMap::iterator start)
{
    assert(start->second.continuous);

    std::queue<FrameMap::iterator> continuous_frames;
    continuous_frames.push(start);

    // A simple BFS to traverse continuous frames.
    while (!continuous_frames.empty()) {
        auto frame = continuous_frames.front();
        continuous_frames.pop();

        if (!last_continuous_frame_ || *last_continuous_frame_ < frame->first) {
            last_continuous_frame_ = frame->first;
        }

        // Loop through all dependent frames, and if that frame no longer has
        // any unfulfilled dependencies then that frame is continuous as well.
        for (size_t d = 0; d < frame->second.dependent_frames.size(); ++d) {
            auto frame_ref = frames_.find(frame->second.dependent_frames[d]);
            assert(frame_ref != frames_.end());

            // TODO(philipel): Look into why we've seen this happen.
            if (frame_ref != frames_.end()) {
                --frame_ref->second.num_missing_continuous;
                if (frame_ref->second.num_missing_continuous == 0) {
                    frame_ref->second.continuous = true;
                    continuous_frames.push(frame_ref);
                }
            }
        }
    }
}

void JitterBuffer::propagate_decodability(const FrameInfo& info)
{
    for (size_t d = 0; d < info.dependent_frames.size(); ++d) {
        auto ref_info = frames_.find(info.dependent_frames[d]);
        assert(ref_info != frames_.end());
        // TODO(philipel): Look into why we've seen this happen.
        if (ref_info != frames_.end()) {
            assert(ref_info->second.num_missing_decodable > 0U);
            --ref_info->second.num_missing_decodable;
        }
    }
}

void JitterBuffer::main_loop()
{
    //TODO: add jitter
    while (true) {
        auto frame_to_decode = find_next_frame();
        auto frame = get_next_frame(frame_to_decode);
        decodale_frames_.send(frame);
    }
}

std::vector<JitterBuffer::FrameMap::iterator> JitterBuffer::find_next_frame()
{
    std::vector<FrameMap::iterator> frames_to_decode;
    int64_t wait_ms = 0;
    //int64_t wait_ms = latest_return_time_ms_ - now_ms;
    // |last_continuous_frame_| may be empty below, but nullopt is smaller
    // than everything else and loop will immediately terminate as expected.
    for (auto frame_it = frames_.begin();
         frame_it != frames_.end() && frame_it->first <= last_continuous_frame_;
         ++frame_it) {
        if (!frame_it->second.continuous || frame_it->second.num_missing_decodable > 0) {
            continue;
        }

        std::optional<ReceivedFrame>& frame = frame_it->second.frame;

        if (keyframe_required_ && !(frame->frame_type == VideoFrameType::VideoFrameKey))
            continue;

        auto last_decoded_frame_timestamp = decoded_frames_history_.GetLastDecodedFrameTimestamp();

        // TODO(https://bugs.webrtc.org/9974): consider removing this check
        // as it may make a stream undecodable after a very long delay between
        // frames.
        if (last_decoded_frame_timestamp && webrtc::AheadOf(*last_decoded_frame_timestamp, frame->timestamp)) {
            continue;
        }

        // Gather all remaining frames for the same superframe.
        std::vector<FrameMap::iterator> current_superframe;
        current_superframe.push_back(frame_it);
        //bool last_layer_completed = frame_it->second.frame->is_last_spatial_layer;
        bool last_layer_completed = true;
        FrameMap::iterator next_frame_it = frame_it;
        while (!last_layer_completed) {
            ++next_frame_it;

            if (next_frame_it == frames_.end() || !next_frame_it->second.frame) {
                break;
            }

            if (next_frame_it->second.frame->timestamp != frame->timestamp || !next_frame_it->second.continuous) {
                break;
            }

            if (next_frame_it->second.num_missing_decodable > 0) {
                bool has_inter_layer_dependency = false;
                for (size_t i = 0; i < kMaxFrameReferences && i < next_frame_it->second.frame->num_references;
                     ++i) {
                    if (next_frame_it->second.frame->references[i] >= frame_it->first) {
                        has_inter_layer_dependency = true;
                        break;
                    }
                }

                // If the frame has an undecoded dependency that is not within the same
                // temporal unit then this frame is not yet ready to be decoded. If it
                // is within the same temporal unit then the not yet decoded dependency
                // is just a lower spatial frame, which is ok.
                if (!has_inter_layer_dependency || next_frame_it->second.num_missing_decodable > 1) {
                    break;
                }
            }

            current_superframe.push_back(next_frame_it);
            //FIXME: uncomment the following line to support other codec
            //last_layer_completed = next_frame_it->second.frame->is_last_spatial_layer;
        }
        // Check if the current superframe is complete.
        // TODO(bugs.webrtc.org/10064): consider returning all available to
        // decode frames even if the superframe is not complete yet.
        if (!last_layer_completed) {
            continue;
        }

        frames_to_decode = std::move(current_superframe);

        //TODO: Render Time
        //if (frame->RenderTime() == -1) {
        //    frame->SetRenderTime(timing_->RenderTimeMs(frame->Timestamp(), now_ms));
        //}
        //wait_ms = timing_->MaxWaitingTime(frame->RenderTime(), now_ms);

        // This will cause the frame buffer to prefer high framerate rather
        // than high resolution in the case of the decoder not decoding fast
        // enough and the stream has multiple spatial and temporal layers.
        // For multiple temporal layers it may cause non-base layer frames to be
        // skipped if they are late.
        if (wait_ms < -kMaxAllowedFrameDelayMs) {
            continue;
        }
        break;
    }
    return frames_to_decode;
}

ReceivedFrame JitterBuffer::get_next_frame(std::vector<FrameMap::iterator>& frames_to_decode)
{
    //FIXME: add jitter
    //int64_t now_ms = clock_->TimeInMilliseconds();
    // TODO(ilnik): remove |frames_out| use frames_to_decode_ directly.
    std::vector<ReceivedFrame> frames_out;

    assert(!frames_to_decode.empty());
    //bool superframe_delayed_by_retransmission = false;
    //size_t superframe_size = 0;
    //EncodedFrame* first_frame = frames_to_decode[0]->second.frame.get();
    //int64_t render_time_ms = first_frame->RenderTime();
    //int64_t receive_time_ms = first_frame->ReceivedTime();
    // Gracefully handle bad RTP timestamps and render time issues.
    //if (HasBadRenderTiming(*first_frame, now_ms)) {
    //    jitter_estimator_.Reset();
    //    timing_->Reset();
    //    render_time_ms = timing_->RenderTimeMs(first_frame->Timestamp(), now_ms);
    //}

    for (FrameMap::iterator& frame_it : frames_to_decode) {
        assert(frame_it != frames_.end());
        std::optional<ReceivedFrame>& frame = frame_it->second.frame;

        //frame->SetRenderTime(render_time_ms);

        //superframe_delayed_by_retransmission |= frame->delayed_by_retransmission();
        //receive_time_ms = std::max(receive_time_ms, frame->ReceivedTime());
        //superframe_size += frame->size();

        propagate_decodability(frame_it->second);
        decoded_frames_history_.InsertDecoded(frame_it->first, frame->timestamp);

        // Remove decoded frame and all undecoded frames before it.
        //if (stats_callback_) {
        //    unsigned int dropped_frames = std::count_if(frames_.begin(), frame_it,
        //        [](const std::pair<const int64_t, FrameInfo>& frame) {
        //            return frame.second.frame != nullptr;
        //        });
        //    if (dropped_frames > 0) {
        //        stats_callback_->OnDroppedFrames(dropped_frames);
        //    }
        //}

        frames_.erase(frames_.begin(), ++frame_it);

        frames_out.push_back(frame.value());
    }

    //if (!superframe_delayed_by_retransmission) {
    //    int64_t frame_delay;

    //    if (inter_frame_delay_.CalculateDelay(first_frame->Timestamp(),
    //            &frame_delay, receive_time_ms)) {
    //        jitter_estimator_.UpdateEstimate(frame_delay, superframe_size);
    //    }

    //    float rtt_mult = protection_mode_ == kProtectionNackFEC ? 0.0 : 1.0;
    //    absl::optional<float> rtt_mult_add_cap_ms = absl::nullopt;
    //    if (rtt_mult_settings_.has_value()) {
    //        rtt_mult = rtt_mult_settings_->rtt_mult_setting;
    //        rtt_mult_add_cap_ms = rtt_mult_settings_->rtt_mult_add_cap_ms;
    //    }
    //    timing_->SetJitterDelay(
    //        jitter_estimator_.GetJitterEstimate(rtt_mult, rtt_mult_add_cap_ms));
    //    timing_->UpdateCurrentDelay(render_time_ms, now_ms);
    //} else {
    //    if (RttMultExperiment::RttMultEnabled() || add_rtt_to_playout_delay_)
    //        jitter_estimator_.FrameNacked();
    //}

    //UpdateJitterDelay();
    //UpdateTimingFrameInfo();

    //TODO: support other codec
    //if (frames_out.size() == 1) {
        return frames_out[0];
    //} else {
    //    return CombineAndDeleteFrames(frames_out);
    //}
}

} // namespace brtc

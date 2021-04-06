#include <variant>
#include "../../common/sequence_number_util.h"
#include "reference_finder.h"
#include "rtp_frame_id_only_ref_finder.h"
#include "rtp_generic_ref_finder.h"
#include "rtp_seq_num_only_ref_finder.h"
#include "rtp_vp8_ref_finder.h"
#include "rtp_vp9_ref_finder.h"

namespace brtc {

class RtpFrameReferenceFinderImpl {
public:
    RtpFrameReferenceFinderImpl() = default;

    RtpFrameReferenceFinder::ReturnVector ManageFrame(
        std::unique_ptr<Frame> frame);
    RtpFrameReferenceFinder::ReturnVector PaddingReceived(uint16_t seq_num);
    void ClearTo(uint16_t seq_num);

private:
    using RefFinder = std::variant<
        RtpGenericFrameRefFinder,
        RtpFrameIdOnlyRefFinder,
        RtpSeqNumOnlyRefFinder,
        RtpVp8RefFinder,
        RtpVp9RefFinder>;

    template <typename T>
    T& GetRefFinderAs();
    RefFinder ref_finder_;
};

RtpFrameReferenceFinder::ReturnVector RtpFrameReferenceFinderImpl::ManageFrame(
    std::unique_ptr<Frame> frame)
{
    const RTPVideoHeader& video_header = frame->GetRtpVideoHeader();

    if (video_header.generic.has_value()) {
        return GetRefFinderAs<RtpGenericFrameRefFinder>().ManageFrame(
            std::move(frame), *video_header.generic);
    }

    switch (frame->codec_type()) {
    case kVideoCodecVP8: {
        const RTPVideoHeaderVP8& vp8_header = absl::get<RTPVideoHeaderVP8>(video_header.video_type_header);

        if (vp8_header.temporalIdx == kNoTemporalIdx || vp8_header.tl0PicIdx == kNoTl0PicIdx) {
            if (vp8_header.pictureId == kNoPictureId) {
                return GetRefFinderAs<RtpSeqNumOnlyRefFinder>().ManageFrame(
                    std::move(frame));
            }

            return GetRefFinderAs<RtpFrameIdOnlyRefFinder>().ManageFrame(
                std::move(frame), vp8_header.pictureId);
        }

        return GetRefFinderAs<RtpVp8RefFinder>().ManageFrame(std::move(frame));
    }
    case kVideoCodecVP9: {
        const RTPVideoHeaderVP9& vp9_header = absl::get<RTPVideoHeaderVP9>(video_header.video_type_header);

        if (vp9_header.temporal_idx == kNoTemporalIdx) {
            if (vp9_header.picture_id == kNoPictureId) {
                return GetRefFinderAs<RtpSeqNumOnlyRefFinder>().ManageFrame(
                    std::move(frame));
            }

            return GetRefFinderAs<RtpFrameIdOnlyRefFinder>().ManageFrame(
                std::move(frame), vp9_header.picture_id);
        }

        return GetRefFinderAs<RtpVp9RefFinder>().ManageFrame(std::move(frame));
    }
    case kVideoCodecGeneric: {
        if (auto* generic_header = absl::get_if<RTPVideoHeaderLegacyGeneric>(
                &video_header.video_type_header)) {
            return GetRefFinderAs<RtpFrameIdOnlyRefFinder>().ManageFrame(
                std::move(frame), generic_header->picture_id);
        }

        return GetRefFinderAs<RtpSeqNumOnlyRefFinder>().ManageFrame(
            std::move(frame));
    }
    default: {
        return GetRefFinderAs<RtpSeqNumOnlyRefFinder>().ManageFrame(
            std::move(frame));
    }
    }
}

RtpFrameReferenceFinder::ReturnVector
RtpFrameReferenceFinderImpl::PaddingReceived(uint16_t seq_num)
{
    if (auto* ref_finder = absl::get_if<RtpSeqNumOnlyRefFinder>(&ref_finder_)) {
        return ref_finder->PaddingReceived(seq_num);
    }
    return {};
}

void RtpFrameReferenceFinderImpl::ClearTo(uint16_t seq_num)
{
    struct ClearToVisitor {
        void operator()(absl::monostate& ref_finder) { }
        void operator()(RtpGenericFrameRefFinder& ref_finder) { }
        void operator()(RtpFrameIdOnlyRefFinder& ref_finder) { }
        void operator()(RtpSeqNumOnlyRefFinder& ref_finder)
        {
            ref_finder.ClearTo(seq_num);
        }
        void operator()(RtpVp8RefFinder& ref_finder)
        {
            ref_finder.ClearTo(seq_num);
        }
        void operator()(RtpVp9RefFinder& ref_finder)
        {
            ref_finder.ClearTo(seq_num);
        }
        uint16_t seq_num;
    };

    absl::visit(ClearToVisitor { seq_num }, ref_finder_);
}

template <typename T>
T& RtpFrameReferenceFinderImpl::GetRefFinderAs()
{
    if (auto* ref_finder = absl::get_if<T>(&ref_finder_)) {
        return *ref_finder;
    }
    return ref_finder_.emplace<T>();
}



RtpFrameReferenceFinder::RtpFrameReferenceFinder(
    OnCompleteFrameCallback* frame_callback)
    : RtpFrameReferenceFinder(frame_callback, 0)
{
}

RtpFrameReferenceFinder::RtpFrameReferenceFinder(
    OnCompleteFrameCallback* frame_callback,
    int64_t picture_id_offset)
    : picture_id_offset_(picture_id_offset)
    , frame_callback_(frame_callback)
    , impl_(std::make_unique<RtpFrameReferenceFinderImpl>())
{
}

RtpFrameReferenceFinder::~RtpFrameReferenceFinder() = default;

void RtpFrameReferenceFinder::ManageFrame(
    std::unique_ptr<Frame> frame)
{
    // If we have cleared past this frame, drop it.
    if (cleared_to_seq_num_ != -1 && webrtc::AheadOf<uint16_t>(cleared_to_seq_num_, frame->first_seq_num())) {
        return;
    }
    HandOffFrames(impl_->ManageFrame(std::move(frame)));
}

void RtpFrameReferenceFinder::PaddingReceived(uint16_t seq_num)
{
    HandOffFrames(impl_->PaddingReceived(seq_num));
}

void RtpFrameReferenceFinder::ClearTo(uint16_t seq_num)
{
    cleared_to_seq_num_ = seq_num;
    impl_->ClearTo(seq_num);
}

void RtpFrameReferenceFinder::HandOffFrames(ReturnVector frames)
{
    for (auto& frame : frames) {
        frame->SetId(frame->Id() + picture_id_offset_);
        for (size_t i = 0; i < frame->num_references; ++i) {
            frame->references[i] += picture_id_offset_;
        }

        frame_callback_->OnCompleteFrame(std::move(frame));
    }
}

} // namespace brtc

#include <variant>
#include "common/sequence_number_util.h"
#include "video/reference_finder/reference_finder.h"
#include "video/reference_finder/rtp_frame_id_only_ref_finder.h"
#include "video/reference_finder/rtp_generic_ref_finder.h"
#include "video/reference_finder/rtp_seq_num_only_ref_finder.h"
#include "video/reference_finder/rtp_vp8_ref_finder.h"
#include "video/reference_finder/rtp_vp9_ref_finder.h"

namespace brtc {

class RtpFrameReferenceFinderImpl {
public:
    RtpFrameReferenceFinderImpl() = default;

    RtpFrameReferenceFinder::ReturnVector ManageFrame(
        std::unique_ptr<ReceivedFrame> frame);
    RtpFrameReferenceFinder::ReturnVector PaddingReceived(uint16_t seq_num);
    void ClearTo(uint16_t seq_num);

private:
    using RefFinder = std::variant<
        webrtc::RtpGenericFrameRefFinder,
        webrtc::RtpFrameIdOnlyRefFinder,
        webrtc::RtpSeqNumOnlyRefFinder,
        webrtc::RtpVp8RefFinder,
        webrtc::RtpVp9RefFinder>;

    template <typename T>
    T& GetRefFinderAs();
    RefFinder ref_finder_;
};

RtpFrameReferenceFinder::ReturnVector RtpFrameReferenceFinderImpl::ManageFrame(
    std::unique_ptr<ReceivedFrame> frame)
{
    const RTPVideoHeader& video_header = std::get<RTPVideoHeader>(frame->video_header);

    if (video_header.generic.has_value()) {
        return GetRefFinderAs<webrtc::RtpGenericFrameRefFinder>().ManageFrame(
            std::move(frame), *video_header.generic);
    }

    switch (frame->codec_type) {
    case VideoCodecType::VP8: {
        const RTPVideoHeaderVP8& vp8_header = std::get<RTPVideoHeaderVP8>(frame->video_header);

        if (vp8_header.temporalIdx == webrtc::kNoTemporalIdx || vp8_header.tl0PicIdx == webrtc::kNoTl0PicIdx) {
            if (vp8_header.pictureId == webrtc::kNoPictureId) {
                return GetRefFinderAs<webrtc::RtpSeqNumOnlyRefFinder>().ManageFrame(
                    std::move(frame));
            }

            return GetRefFinderAs<webrtc::RtpFrameIdOnlyRefFinder>().ManageFrame(
                std::move(frame), vp8_header.pictureId);
        }

        return GetRefFinderAs<webrtc::RtpVp8RefFinder>().ManageFrame(std::move(frame));
    }
    case VideoCodecType::VP9: {
        const RTPVideoHeaderVP9& vp9_header = std::get<RTPVideoHeaderVP9>(frame->video_header);

        if (vp9_header.temporal_idx == webrtc::kNoTemporalIdx) {
            if (vp9_header.picture_id == webrtc::kNoPictureId) {
                return GetRefFinderAs<webrtc::RtpSeqNumOnlyRefFinder>().ManageFrame(
                    std::move(frame));
            }

            return GetRefFinderAs<webrtc::RtpFrameIdOnlyRefFinder>().ManageFrame(
                std::move(frame), vp9_header.picture_id);
        }

        return GetRefFinderAs<webrtc::RtpVp9RefFinder>().ManageFrame(std::move(frame));
    }
    default: {
        return GetRefFinderAs<webrtc::RtpSeqNumOnlyRefFinder>().ManageFrame(
            std::move(frame));
    }
    }
}

RtpFrameReferenceFinder::ReturnVector
RtpFrameReferenceFinderImpl::PaddingReceived(uint16_t seq_num)
{
    return std::get<webrtc::RtpSeqNumOnlyRefFinder>(ref_finder_).PaddingReceived(seq_num);

}

void RtpFrameReferenceFinderImpl::ClearTo(uint16_t seq_num)
{
    struct ClearToVisitor {
        void operator()(webrtc::RtpGenericFrameRefFinder&) { }
        void operator()(webrtc::RtpFrameIdOnlyRefFinder&) { }
        void operator()(webrtc::RtpSeqNumOnlyRefFinder& ref_finder)
        {
            ref_finder.ClearTo(seq_num);
        }
        void operator()(webrtc::RtpVp8RefFinder& ref_finder)
        {
            ref_finder.ClearTo(seq_num);
        }
        void operator()(webrtc::RtpVp9RefFinder& ref_finder)
        {
            ref_finder.ClearTo(seq_num);
        }
        uint16_t seq_num;
    };

    std::visit(ClearToVisitor { seq_num }, ref_finder_);
}

template <typename T>
T& RtpFrameReferenceFinderImpl::GetRefFinderAs()
{
    if (T* ref_finder = std::get_if<T>(&ref_finder_)) {
        return *ref_finder;
    }
    return ref_finder_.emplace<T>();
}



RtpFrameReferenceFinder::RtpFrameReferenceFinder()
    : RtpFrameReferenceFinder(0)
{
}

RtpFrameReferenceFinder::RtpFrameReferenceFinder(int64_t picture_id_offset)
    : picture_id_offset_(picture_id_offset)
    , impl_(std::make_unique<RtpFrameReferenceFinderImpl>())
{
}

RtpFrameReferenceFinder::~RtpFrameReferenceFinder() = default;

void RtpFrameReferenceFinder::ManageFrame(std::unique_ptr<ReceivedFrame> frame)
{
    // If we have cleared past this frame, drop it.
    if (cleared_to_seq_num_ != -1 && webrtc::AheadOf<uint16_t>(static_cast<uint16_t>(cleared_to_seq_num_), frame->first_seq_num)) {
        return;
    }
    auto frames = impl_->ManageFrame(std::move(frame));
    for (auto& f : frames) {
        f->id = f->id + picture_id_offset_;
        for (size_t i = 0; i < f->num_references; ++i) {
            f->references[i] += picture_id_offset_;
            frames_.push(std::move(f));
        }
    }
}

RtpFrameReferenceFinder::ReturnVector RtpFrameReferenceFinder::PaddingReceived(uint16_t seq_num)
{
    return impl_->PaddingReceived(seq_num);
}

void RtpFrameReferenceFinder::ClearTo(uint16_t seq_num)
{
    cleared_to_seq_num_ = seq_num;
    impl_->ClearTo(seq_num);
}

std::unique_ptr<ReceivedFrame> RtpFrameReferenceFinder::pop_gop_inter_continous_frame()
{
    auto frame = std::move(frames_.front());
    frames_.pop();
    return std::move(frame);
}

} // namespace brtc

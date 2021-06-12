#include <array>
#include <thread>

#include <glog/logging.h>

#include "common/time_utils.h"
#include "mfx_encoder.h"

namespace brtc {

namespace builtin {

using Microsoft::WRL::ComPtr;

MfxEncoder::~MfxEncoder()
{
    if (mfx_session_ != nullptr)
        MFXClose(mfx_session_);
}

bool MfxEncoder::init(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    device_ = device;
    allocator_ = std::make_shared<MfxEncoderFrameAllocator>(device);
    mfxVersion ver = { { 0, 1 } };
    //FIXEME set impl to previous impl
    mfxIMPL impl = MFX_IMPL_HARDWARE | MFX_IMPL_VIA_D3D11;
    mfxStatus status = MFXInit(impl, &ver, &mfx_session_);
    if (status != MFX_ERR_NONE) {
        mfx_session_ = nullptr;
        return false;
    }
    MFXQueryIMPL(mfx_session_, &impl);
    MFXQueryVersion(mfx_session_, &ver);
    status = MFXVideoCORE_SetHandle(mfx_session_, MFX_HANDLE_D3D11_DEVICE, device_.Get());
    if (status != MFX_ERR_NONE) {
        return false;
    }
    status = MFXVideoCORE_SetFrameAllocator(mfx_session_, allocator_.get());
    if (status != MFX_ERR_NONE) {
        return false;
    }
    if (!init_encoder()) {
        return false;
    }
    if (!init_vpp()) {
        return false;
    }
    return true;
}

EncodedFrame MfxEncoder::encode_one_frame(Frame in_frame)
{
    //TODO: add codec specific info
    EncodedFrame out_frame;
    mfxFrameSurface1 vppin, vppout;
    memset(&vppin, 0, sizeof(mfxFrameSurface1));
    memset(&vppout, 0, sizeof(mfxFrameSurface1));
    mfxBitstream bs;
    mfxSyncPoint syncp_encode, syncp_vpp;
    auto data_holder = std::make_shared<std::vector<uint8_t>>(encode_param_.mfx.BufferSizeInKB * 1000);
    memset(&bs, 0, sizeof(bs));
    bs.Data = data_holder->data();
    bs.MaxLength = encode_param_.mfx.BufferSizeInKB * 1000;

    vppin.Data.MemId = in_frame.data;
    vppin.Info = vpp_param_.vpp.In;

    vppout.Data.MemId = render_surface_.Get();
    vppout.Info = vpp_param_.vpp.Out;
    ;
    mfxStatus status = MFXVideoVPP_RunFrameVPPAsync(mfx_session_, &vppin, &vppout, nullptr, &syncp_vpp);
    if (status != MFX_ERR_NONE) {
        return out_frame;
    }
    status = MFXVideoCORE_SyncOperation(mfx_session_, syncp_vpp, 1000);
    if (status != MFX_ERR_NONE) {
        return out_frame;
    }
    status = MFXVideoENCODE_EncodeFrameAsync(mfx_session_, nullptr, &vppout, &bs, &syncp_encode);
    if (status != MFX_ERR_NONE) {
        return out_frame;
    }
    status = MFX_WRN_IN_EXECUTION;
    while (status > 0) {
        status = MFXVideoCORE_SyncOperation(mfx_session_, syncp_encode, 1000);
        if (status < MFX_ERR_NONE) {
            return out_frame;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
    }
    // 记得delete [] out_frame.data
    out_frame.data = data_holder->data();
    out_frame.underlying_type = Frame::UnderlyingType::kMemory;
    out_frame.length = bs.DataLength;
    out_frame.width = vppin.Info.Width;
    out_frame.height = vppin.Info.Height;
    out_frame.timestamp = static_cast<uint32_t>(brtc::MachineNowMilliseconds());
    out_frame._data_holder = data_holder;
    CodecSpecificInfo codec_info;
    codec_info.codecType = VideoCodecType::H264;
    codec_info.codecSpecific.H264 = CodecSpecificInfoH264 {
        .temporal_idx = kNoTemporalIdx,
        .base_layer_sync = false,
    };
    out_frame.codec_info = codec_info;
    return out_frame;
}

bool MfxEncoder::init_encoder()
{
    auto params = gen_encode_param();
    mfxStatus status = MFXVideoENCODE_Query(mfx_session_, &params, &params);
    if (status != MFX_ERR_NONE) {
        return false;
    }
    mfxFrameAllocRequest request;
    status = MFXVideoENCODE_QueryIOSurf(mfx_session_, &params, &request);
    if (status != MFX_ERR_NONE) {
        return false;
    }
    status = MFXVideoENCODE_Init(mfx_session_, &params);
    if (status != MFX_ERR_NONE) {
        return false;
    }
    mfxVideoParam param_out;
    memset(&param_out, 0, sizeof(param_out));
    status = MFXVideoENCODE_GetVideoParam(mfx_session_, &param_out);
    if (status != MFX_ERR_NONE) {
        return false;
    }
    encode_param_ = param_out;
    return true;
}
bool MfxEncoder::init_vpp()
{
    mfxVideoParam params = gen_vpp_param();
    std::array<mfxFrameAllocRequest, 2> requests;
    memset(requests.data(), 0, requests.size() * sizeof(mfxFrameAllocRequest));
    //这步其实可以不需要
    mfxStatus status = MFXVideoVPP_QueryIOSurf(mfx_session_, &params, requests.data());
    if (status != MFX_ERR_NONE) {
        return false;
    }
    render_surface_ = alloc_render_surface();
    status = MFXVideoVPP_Init(mfx_session_, &params);
    if (status != MFX_ERR_NONE) {
        return false;
    }
    vpp_param_ = params;
    return true;
}
ComPtr<ID3D11Texture2D> MfxEncoder::alloc_render_surface()
{
    ComPtr<ID3D11Texture2D> frame;
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = 1920;
    desc.Height = 1088; //这里填1080会出问题
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Format = DXGI_FORMAT_NV12;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = 0;
    //desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, frame.GetAddressOf());
    if (FAILED(hr)) {
        //assert(false);
        return nullptr;
    }
    return frame;
}

mfxVideoParam MfxEncoder::gen_encode_param()
{
#define MSDK_ALIGN16(value) (((value + 15) >> 4) << 4)
#define MSDK_ALIGN32(X) (((mfxU32)((X) + 31)) & (~(mfxU32)31))
    mfxVideoParam params;
    memset(&params, 0, sizeof(params));
    params.mfx.CodecId = MFX_CODEC_AVC;
    params.mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
    params.mfx.TargetKbps = 3 * 1024 * 8;
    params.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
    params.mfx.FrameInfo.FrameRateExtN = 30;
    params.mfx.FrameInfo.FrameRateExtD = 1;
    params.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    params.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    params.mfx.FrameInfo.CropX = 0;
    params.mfx.FrameInfo.CropY = 0;
    params.mfx.FrameInfo.CropW = 1920;
    params.mfx.FrameInfo.CropH = 1080;
    params.mfx.FrameInfo.Width = MSDK_ALIGN16(1920);
    params.mfx.FrameInfo.Height = MSDK_ALIGN32(1080);
    params.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    params.AsyncDepth = 1;
    params.mfx.GopRefDist = 1;
    return params;
#undef MSDK_ALIGN32
#undef MSDK_ALIGN16
}
mfxVideoParam MfxEncoder::gen_vpp_param()
{
    mfxVideoParam params;
    memset(&params, 0, sizeof(params));
#define MSDK_ALIGN16(value) (((value + 15) >> 4) << 4)
#define MSDK_ALIGN32(X) (((mfxU32)((X) + 31)) & (~(mfxU32)31))
    params.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    // Input data
    params.vpp.In.FourCC = MFX_FOURCC_RGB4;
    params.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
    params.vpp.In.CropX = 0;
    params.vpp.In.CropY = 0;
    params.vpp.In.CropW = 1920;
    params.vpp.In.CropH = 1080;
    params.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    params.vpp.In.FrameRateExtN = 30;
    params.vpp.In.FrameRateExtD = 1;
    params.vpp.In.Width = MSDK_ALIGN16(1920);
    params.vpp.In.Height = MSDK_ALIGN16(1080);
    // Output data
    params.vpp.Out.FourCC = MFX_FOURCC_NV12;
    params.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    params.vpp.Out.CropX = 0;
    params.vpp.Out.CropY = 0;
    params.vpp.Out.CropW = 1920;
    params.vpp.Out.CropH = 1080;
    params.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    params.vpp.Out.FrameRateExtN = 30;
    params.vpp.Out.FrameRateExtD = 1;
    params.vpp.Out.Width = MSDK_ALIGN16(1920);
    params.vpp.Out.Height = MSDK_ALIGN16(1080);

#undef MSDK_ALIGN32
#undef MSDK_ALIGN16
    return params;
}

} // namespace builtin

} // namespace brtc

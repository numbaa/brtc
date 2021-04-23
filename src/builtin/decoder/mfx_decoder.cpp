#include <array>
#include <iostream>

#include <mfxvideo.h>

#include "mfx_decoder.h"

namespace brtc {

namespace builtin {

using Microsoft::WRL::ComPtr;


bool MfxDecoder::init(ComPtr<ID3D11Device> device)
{
    device_ = device;
    allocator_ = std::make_shared<MfxDecoderFrameAllocator>(device);
    mfxVersion ver = { { 0, 1 } };
    //TODO use previous impl
    mfxIMPL impl = MFX_IMPL_HARDWARE | MFX_IMPL_VIA_D3D11;
    mfxStatus status = MFXInit(impl, &ver, &mfx_session_);
    if (status != MFX_ERR_NONE) {
        return false;
    }
    status = MFXVideoCORE_SetHandle(mfx_session_, MFX_HANDLE_D3D11_DEVICE, device_.Get());
    if (status != MFX_ERR_NONE) {
        return false;
    }
    status = MFXVideoCORE_SetFrameAllocator(mfx_session_, allocator_.get());
    if (status != MFX_ERR_NONE) {
        return false;
    }
    return true;
}

int MfxDecoder::init2(Frame encoded_frame)
{
    int offset = 0;
    mfxBitstream bs;
    memset(&bs, 0, sizeof(mfxBitstream));
    bs.MaxLength = encoded_frame.length;
    bs.Data = (mfxU8*)encoded_frame.data;
    bs.DataLength = encoded_frame.length;
    mfxVideoParam params;
    memset(&params, 0, sizeof(mfxVideoParam));
    params.mfx.CodecId = MFX_CODEC_AVC;
    params.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    mfxStatus status = MFXVideoDECODE_DecodeHeader(mfx_session_, &bs, &params);
    if (status != MFX_ERR_NONE) {
        return 0;
    }
    offset = bs.DataOffset;
    decode_param_ = params;
    mfxFrameAllocRequest request;
    status = MFXVideoDECODE_QueryIOSurf(mfx_session_, &params, &request);
    if (status != MFX_ERR_NONE) {
        return 0;
    }
    //这步可以省略让init自动去做
    mfxFrameAllocResponse response;
    status = allocator_->alloc(&request, &response);
    if (status != MFX_ERR_NONE) {
        return 0;
    }
    status = MFXVideoDECODE_Init(mfx_session_, &params);
    if (status != MFX_ERR_NONE) {
        return 0;
    }
    surfaces_.resize(response.NumFrameActual);
    for (int i = 0; i < response.NumFrameActual; i++) {
        memset(&surfaces_[i], 0, sizeof(mfxFrameSurface1));
        surfaces_[i].Info = params.mfx.FrameInfo;
        surfaces_[i].Data.MemId = response.mids[i];
    }
    already_init_ = true;
    return offset;
}

Frame MfxDecoder::decode_one_frame(Frame encoded_frame)
{
    Frame decoded_frame;
    int offset = 0;
    if (!already_init_) {
        offset = init2(encoded_frame);
        if (offset == 0)
            return decoded_frame;
    }
    int32_t index = get_unlocked_frame();
    if (index == std::numeric_limits<int32_t>::max()) {
        std::cout << "No more available frames" << std::endl;
        return decoded_frame;
    }
    mfxFrameSurface1* surface_out;
    mfxSyncPoint syncp;
    mfxBitstream bs;
    memset(&bs, 0, sizeof(bs));
    bs.DataLength = encoded_frame.length - offset;
    bs.MaxLength = encoded_frame.length;
    bs.Data = (mfxU8*)encoded_frame.data;
    bs.DataOffset = offset;
    mfxFrameSurface1* surface_work = &surfaces_[index];
    mfxMemId mid = surface_work->Data.MemId;
    ID3D11Texture2D* t2d = reinterpret_cast<ID3D11Texture2D*>(mid);
    D3D11_TEXTURE2D_DESC desc { 0 };
    t2d->GetDesc(&desc);
    mfxStatus status = MFXVideoDECODE_DecodeFrameAsync(mfx_session_, &bs, surface_work, &surface_out, &syncp);
    if (status == MFX_ERR_NONE) {
        status = MFXVideoCORE_SyncOperation(mfx_session_, syncp, 100);
        if (status == MFX_ERR_NONE) {
            delete encoded_frame.data;
            decoded_frame.data = surface_out->Data.MemId;
            decoded_frame.type = Frame::UnderlyingType::kD3D11Texture2D;
            decoded_frame.height = surface_out->Info.Height;
            decoded_frame.width = surface_out->Info.Width;
            return decoded_frame;
        }
        delete encoded_frame.data;
        return decoded_frame;
        //sync and present surface_out, unlock?
    }
    if (status == MFX_ERR_MORE_DATA || status == MFX_ERR_MORE_SURFACE) {
        delete encoded_frame.data;
        return decoded_frame;
    }
    //fatal error
    delete encoded_frame.data;
    return decoded_frame;
}

mfxStatus MfxDecoder::release_frame(ComPtr<ID3D11Texture2D> frame)
{
    mfxFrameSurface1* surface = nullptr; // = find_surface_by_texture2d(frame);
    surface->Data.Locked -= 1;
    return MFX_ERR_NONE;
}

int32_t MfxDecoder::get_unlocked_frame()
{
    auto it = std::find_if(surfaces_.begin(), surfaces_.end(), [](const mfxFrameSurface1& surf) {
        return !surf.Data.Locked;
    });
    if (it == surfaces_.end()) {
        return std::numeric_limits<int32_t>::max();
    } else {
        return it - surfaces_.begin();
    }
}

} // namespace builtin

} // namespace brtc

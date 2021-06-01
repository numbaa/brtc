#pragma once
#include <exception>
#include <glog/logging.h>
#include "mfx_allocator.h"

namespace brtc {

namespace builtin {

using Microsoft::WRL::ComPtr;

mfxStatus MFX_CDECL MfxFrameAllocator::_alloc(mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    return reinterpret_cast<MfxFrameAllocator*>(pthis)->alloc(request, response);
}

mfxStatus MFX_CDECL MfxFrameAllocator::_lock(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
{
    return reinterpret_cast<MfxFrameAllocator*>(pthis)->lock(mid, ptr);
}

mfxStatus MFX_CDECL MfxFrameAllocator::_unlock(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
{
    return reinterpret_cast<MfxFrameAllocator*>(pthis)->unlock(mid, ptr);
}

mfxStatus MFX_CDECL MfxFrameAllocator::_getHDL(mfxHDL pthis, mfxMemId mid, mfxHDL* handle)
{
    return reinterpret_cast<MfxFrameAllocator*>(pthis)->get_hdl(mid, handle);
}

mfxStatus MFX_CDECL MfxFrameAllocator::_free(mfxHDL pthis, mfxFrameAllocResponse* response)
{
    return reinterpret_cast<MfxFrameAllocator*>(pthis)->free(response);
}

MfxFrameAllocator::MfxFrameAllocator()
{
    this->pthis = this;
    this->Alloc = this->_alloc;
    this->Free = this->_free;
    this->GetHDL = this->_getHDL;
    this->Lock = this->_lock;
    this->Unlock = this->_unlock;
}

/*

MfxD3D11Allocator::MfxD3D11Allocator(Microsoft::WRL::ComPtr<ID3D11Device> device)
    : device_(device)
{
}

mfxStatus MfxD3D11Allocator::alloc(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    if (request->Type & MFX_MEMTYPE_EXTERNAL_FRAME && request->Type & MFX_MEMTYPE_FROM_DECODE) {
        return alloc_external_frame(request, response);
    } else {
        return alloc_internal_frame(request, response);
    }
}

mfxStatus MfxD3D11Allocator::lock(mfxMemId mid, mfxFrameData* ptr)
{
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MfxD3D11Allocator::unlock(mfxMemId mid, mfxFrameData* ptr)
{
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MfxD3D11Allocator::get_hdl(mfxMemId mid, mfxHDL* handle)
{
    auto pair = reinterpret_cast<mfxHDLPair*>(handle);
    pair->first = mid;
    pair->second = 0;
    return MFX_ERR_NONE;
}

mfxStatus MfxD3D11Allocator::free(mfxFrameAllocResponse* response)
{
    if (response->mids == external_frames_.mids) {
        //
    } else {
    }
    return MFX_ERR_NONE;
}

mfxStatus MfxD3D11Allocator::alloc_external_frame(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    if (external_frames_.mids != nullptr) {
        response->mids = external_frames_.mids;
        response->NumFrameActual = external_frames_.frames.size();
        return MFX_ERR_NONE;
    }
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = request->Info.Width;
    desc.Height = request->Info.Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_NV12;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_DECODER;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.MiscFlags = 0;
    external_frames_.mids = static_cast<mfxMemId*>(malloc(request->NumFrameSuggested * sizeof(mfxMemId)));
    for (int i = 0; i < request->NumFrameSuggested; i++) {
        ComPtr<ID3D11Texture2D> frame;
        HRESULT hr = device_->CreateTexture2D(&desc, nullptr, frame.GetAddressOf());
        if (FAILED(hr)) {
            return MFX_ERR_MEMORY_ALLOC;
        }
        external_frames_.mids[i] = frame.Get();
        external_frames_.frames.push_back(frame);
    }
    response->NumFrameActual = request->NumFrameSuggested;
    response->mids = external_frames_.mids;
    return MFX_ERR_NONE;
}

mfxStatus MfxD3D11Allocator::alloc_internal_frame(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    FrameBuffer fb;
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = request->Info.Width;
    desc.Height = request->Info.Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Format = DXGI_FORMAT_NV12;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = 0;
    fb.mids = static_cast<mfxMemId*>(malloc(request->NumFrameSuggested * sizeof(mfxMemId)));
    for (int i = 0; i < request->NumFrameSuggested; i++) {
        ComPtr<ID3D11Texture2D> frame;
        HRESULT hr = device_->CreateTexture2D(&desc, nullptr, frame.GetAddressOf());
        if (FAILED(hr)) {
            return MFX_ERR_MEMORY_ALLOC;
        }
        fb.mids[i] = frame.Get();
        fb.frames.push_back(frame);
    }
    internal_frames_[fb.mids] = std::move(fb);
    response->NumFrameActual = request->NumFrameSuggested;
    response->mids = fb.mids;
    return MFX_ERR_NONE;
}

*/

MfxEncoderFrameAllocator::MfxEncoderFrameAllocator(Microsoft::WRL::ComPtr<ID3D11Device> device)
    : device_(device)
{
}

mfxStatus MfxEncoderFrameAllocator::alloc(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    FrameBuffer fb;
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = 1920;
    desc.Height = 1080;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_NV12;
    desc.SampleDesc.Count = 1;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    fb.mids = static_cast<mfxMemId*>(malloc(request->NumFrameSuggested * sizeof(mfxMemId)));
    for (int i = 0; i < request->NumFrameSuggested; i++) {
        ComPtr<ID3D11Texture2D> frame;
        HRESULT hr = device_->CreateTexture2D(&desc, nullptr, frame.GetAddressOf());
        if (FAILED(hr)) {
            return MFX_ERR_MEMORY_ALLOC;
        }
        fb.mids[i] = frame.Get();
        fb.frames.push_back(frame);
    }
    frame_buffers_[fb.mids] = std::move(fb);
    response->NumFrameActual = request->NumFrameSuggested;
    response->mids = fb.mids;
    return MFX_ERR_NONE;
}

mfxStatus MfxEncoderFrameAllocator::lock(mfxMemId mid, mfxFrameData* ptr)
{
    //throw std::exception { "MfxEncoderFrameAllocator::lock()" };
    (void)mid;
    (void)ptr;
    LOG(INFO) << "MfxEncoderFrameAllocator::lock()";
    return MFX_ERR_NONE;
}

mfxStatus MfxEncoderFrameAllocator::unlock(mfxMemId mid, mfxFrameData* ptr)
{
    //throw std::exception { "MfxEncoderFrameAllocator::unlock()" };
    (void)mid;
    (void)ptr;
    LOG(INFO) << "MfxEncoderFrameAllocator::unlock()";
    return MFX_ERR_NONE;
}

mfxStatus MfxEncoderFrameAllocator::get_hdl(mfxMemId mid, mfxHDL* handle)
{
    *handle = mid;
    return MFX_ERR_NONE;
}

mfxStatus MfxEncoderFrameAllocator::free(mfxFrameAllocResponse* response)
{
    frame_buffers_.erase(response->mids);
    return MFX_ERR_NONE;
}

MfxDecoderFrameAllocator::MfxDecoderFrameAllocator(Microsoft::WRL::ComPtr<ID3D11Device> device)
    : device_(device)
{
}

mfxStatus MfxDecoderFrameAllocator::alloc(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    if (request->Type & MFX_MEMTYPE_EXTERNAL_FRAME && request->Type & MFX_MEMTYPE_FROM_DECODE) {
        return alloc_external_frame(request, response);
    } else {
        return alloc_internal_frame(request, response);
    }
}

mfxStatus MfxDecoderFrameAllocator::lock(mfxMemId mid, mfxFrameData* ptr)
{
    LOG(INFO) << "MfxDecoderFrameAllocator::lock()";
    (void)mid;
    (void)ptr;
    return MFX_ERR_NONE;
}

mfxStatus MfxDecoderFrameAllocator::unlock(mfxMemId mid, mfxFrameData* ptr)
{
    LOG(INFO) << "MfxDecoderFrameAllocator::unlock()";
    (void)mid;
    (void)ptr;
    return MFX_ERR_NONE;
}

mfxStatus MfxDecoderFrameAllocator::get_hdl(mfxMemId mid, mfxHDL* handle)
{
    mfxHDLPair* pair = (mfxHDLPair*)handle;
    pair->first = mid;
    pair->second = 0;
    return MFX_ERR_NONE;
}

mfxStatus MfxDecoderFrameAllocator::free(mfxFrameAllocResponse* response)
{
    LOG(INFO) << "MfxDecoderFrameAllocator::free()";
    (void)response;
    return MFX_ERR_NONE;
}

mfxStatus MfxDecoderFrameAllocator::release_frame(ComPtr<ID3D11Texture2D> frame)
{
    LOG(INFO) << "MfxDecoderFrameAllocator::release_frame()";
    (void)frame;
    return MFX_ERR_NONE;
}

mfxStatus MfxDecoderFrameAllocator::alloc_external_frame(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    if (external_frames_.mids != nullptr) {
        response->mids = external_frames_.mids;
        response->NumFrameActual = external_frames_.frames.size();
        return MFX_ERR_NONE;
    }
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = 1920;
    desc.Height = 1088;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    //decodeӦ�������ָ�ʽ
    desc.Format = DXGI_FORMAT_NV12;
    desc.SampleDesc.Count = 1;
    //desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.BindFlags = D3D11_BIND_DECODER; //maybe change to decoder
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.MiscFlags = 0;
    external_frames_.mids = static_cast<mfxMemId*>(malloc(request->NumFrameSuggested * sizeof(mfxMemId)));
    for (int i = 0; i < request->NumFrameSuggested; i++) {
        ComPtr<ID3D11Texture2D> frame;
        HRESULT hr = device_->CreateTexture2D(&desc, nullptr, frame.GetAddressOf());
        if (FAILED(hr)) {
            return MFX_ERR_MEMORY_ALLOC;
        }
        external_frames_.mids[i] = frame.Get();
        external_frames_.frames.push_back(frame);
    }
    response->NumFrameActual = request->NumFrameSuggested;
    response->mids = external_frames_.mids;
    return MFX_ERR_NONE;
}

mfxStatus MfxDecoderFrameAllocator::alloc_internal_frame(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    FrameBuffer fb;
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = 1920;
    desc.Height = 1088; //������1080�������
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Format = DXGI_FORMAT_NV12;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = 0;
    //desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    fb.mids = static_cast<mfxMemId*>(malloc(request->NumFrameSuggested * sizeof(mfxMemId)));
    for (int i = 0; i < request->NumFrameSuggested; i++) {
        ComPtr<ID3D11Texture2D> frame;
        HRESULT hr = device_->CreateTexture2D(&desc, nullptr, frame.GetAddressOf());
        if (FAILED(hr)) {
            return MFX_ERR_MEMORY_ALLOC;
        }
        fb.mids[i] = frame.Get();
        fb.frames.push_back(frame);
    }
    internal_frames_[fb.mids] = std::move(fb);
    response->NumFrameActual = request->NumFrameSuggested;
    response->mids = fb.mids;
    return MFX_ERR_NONE;
}

} // namespace builtin

} // namespace brtc
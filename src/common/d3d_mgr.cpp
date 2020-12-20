#pragma once
#include <dxgi1_2.h>

#include <array>

#include <glog/logging.h>
#include <mfxvideo.h>

#include "d3d_mgr.h"

namespace brtc {

using Microsoft::WRL::ComPtr;

struct AutoCloser {
    AutoCloser(mfxSession session)
        : session_(session)
    {
    }
    ~AutoCloser() { MFXClose(session_); }
    mfxSession session_;
};

static int get_adapter_num(mfxSession session, mfxIMPL impl)
{
    //use MFXQueryAdaptersNumber
    MFXQueryIMPL(session, &impl);
    mfxIMPL base_impl = MFX_IMPL_BASETYPE(impl);
    switch (base_impl) {
    case MFX_IMPL_HARDWARE:
        return 0;
    case MFX_IMPL_HARDWARE2:
        return 1;
    case MFX_IMPL_HARDWARE3:
        return 2;
    case MFX_IMPL_HARDWARE4:
        return 3;
    default:
        return -1;
    }
}

bool D3DMgr::init(GpuVender vender)
{
    if (vender != GpuVender::Intel) {
        LOG(ERROR) << "Unsupported GPU";
        return false;
    }
    mfxVersion ver = { { 0, 1 } };
    mfxIMPL impl = MFX_IMPL_HARDWARE | MFX_IMPL_VIA_D3D11;
    mfxSession session;
    mfxStatus status = MFXInit(impl, &ver, &session);
    if (status != MFX_ERR_NONE) {
        return false;
    }
    AutoCloser closer { session };
    int num = get_adapter_num(session, impl);
    if (num == -1) {
        return false;
    }
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGIAdapter> adapter;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)(factory.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }
    hr = factory->EnumAdapters(num, adapter.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    DXGI_ADAPTER_DESC dxgi_desc;
    adapter->GetDesc(&dxgi_desc);
    LOG(INFO) << ", " << std::hex
              << dxgi_desc.VendorId << ":" << dxgi_desc.DeviceId << std::endl;
    return create_device(adapter);
}

ComPtr<ID3D11Device> D3DMgr::get_device()
{
    return device_;
}

bool D3DMgr::create_device(Microsoft::WRL::ComPtr<IDXGIAdapter> adapter)
{
    Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = nullptr;
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    std::array<D3D_FEATURE_LEVEL, 2> feature_levels {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL feature_level_out;
    HRESULT hr = D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        flags,
        feature_levels.data(),
        feature_levels.size(),
        D3D11_SDK_VERSION,
        &device,
        &feature_level_out,
        &context);
    if (FAILED(hr)) {
        return false;
    }
    ComPtr<ID3D10Multithread> mt;
    hr = context.As(&mt);
    mt->SetMultithreadProtected(true);
    device_ = device;
    return true;
}

} // namespace brtc

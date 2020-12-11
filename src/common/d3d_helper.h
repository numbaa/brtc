#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace brtc {

inline auto CreateD3D()
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
        nullptr,
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        feature_levels.data(),
        feature_levels.size(),
        D3D11_SDK_VERSION,
        &device,
        &feature_level_out, //&feature_level_out
        &context);
    Microsoft::WRL::ComPtr<ID3D10Multithread> mt;
    hr = context.As(&mt);
    mt->SetMultithreadProtected(true);
    return std::make_tuple(device, context);
}

}
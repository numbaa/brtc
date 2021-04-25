#include "d3d_helper.h"
#include <dxgi1_2.h>
#include <d3d11.h>
#include <array>
#include <mfxvideo.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace {

UINT get_vendor_id(GpuVendor vendor)
{
    switch (vendor) {
    case GpuVendor::Intel:
        return 0x8086;
    case GpuVendor::Amd:
        return 0x1002;
        break;
    case GpuVendor::Nvida:
        return 0x10de;
    default:
        return 0;
    }
}

Microsoft::WRL::ComPtr<ID3D11Device> create_device(ComPtr<IDXGIAdapter> adapter)
{
    ComPtr<ID3D11Device> device = nullptr;
    ComPtr<ID3D11DeviceContext> context = nullptr;
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
        return nullptr;
    }
    ComPtr<ID3D10Multithread> mt;
    hr = context.As(&mt);
    mt->SetMultithreadProtected(true);
    return device;
}

} // namespace

Microsoft::WRL::ComPtr<ID3D11Device> get_d3d11_device(GpuVendor vendor)
{
    ComPtr<IDXGIFactory> factory;
    ComPtr<IDXGIAdapter> adapter;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)(factory.GetAddressOf()));
    if (FAILED(hr)) {
        return nullptr;
    }
    UINT i = 0;
    while (factory->EnumAdapters(i, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc {};
        adapter->GetDesc(&desc);
        if (desc.VendorId == get_vendor_id(vendor)) {
            return create_device(adapter);
        }
        i++;
    }
    return nullptr;
}

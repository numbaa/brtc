#include <iostream>
#include "dxgi_capture.h"

namespace brtc {
bool DxgiCapture::init()
{
    HRESULT r = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &d3d11_device_,
        nullptr,
        &d3d11_context_);
    if (FAILED(r)) {
        std::cerr << "Create D3D11Device failed\n";
        return false;
    }
    IDXGIDevice* dxgi_device;
    r = d3d11_device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));
    if (FAILED(r)) {
        std::cerr << "Query DXGI device failed\n";
        return false;
    }

    IDXGIAdapter* dxgi_adapter;
    r = dxgi_device->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgi_adapter));
    if (FAILED(r)) {
        std::cerr << "Get DXGI adapter failed\n";
        return false;
    }
    dxgi_device->Release();

    r = dxgi_adapter->EnumOutputs(0, &dxgi_output_);
    if (FAILED(r)) {
        std::cerr << "Enum outputs failed\n";
        return false;
    }
    dxgi_adapter->Release();

    //shenmegui
    IDXGIOutput1* dxgi_output1;
    r = dxgi_output_->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&dxgi_output1));
    if (FAILED(r)) {
        std::cerr << "Query DXGIOutput1 failed\n";
        return false;
    }
    r = dxgi_output1->DuplicateOutput(d3d11_device_, &dxgi_dup_);
    if (FAILED(r)) {
        std::cerr << "Duplicate output failed\n";
        return false;
    }
    dxgi_output1->Release();
    return true;
}
void DxgiCapture::start()
{
}
void DxgiCapture::stop()
{
}
void DxgiCapture::add_observer(VideoSourceObserver* observer)
{
}
ID3D11Texture2D* DxgiCapture::get_frame()
{
    DXGI_OUTDUPL_FRAME_INFO frame_info {};
    IDXGIResource* desktop_resource { nullptr };
    HRESULT hr = dxgi_dup_->AcquireNextFrame(500, &frame_info, &desktop_resource);
    if (FAILED(hr) || hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return nullptr;
    }
    ID3D11Texture2D* frame { nullptr };
    if (frame_info.TotalMetadataBufferSize != 0) {
        hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frame));
        if (FAILED(hr)) {
            std::cerr << "Query desktop texture2d failed\n";
            desktop_resource->Release();
            return nullptr;
        }
    }
    desktop_resource->Release();
    return frame;
}
}
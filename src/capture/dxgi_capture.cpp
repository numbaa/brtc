#include <iostream>
#include "dxgi_capture.h"
#include "../common/d3d_helper.h"

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace brtc {

bool DxgiCapture::init()
{
    std::tie(device_, context_) = CreateD3D();
    ComPtr<IDXGIDevice> dxgi;
    HRESULT hr = device_.As(&dxgi);
    if (FAILED(hr)) {
        return false;
    }
    ComPtr<IDXGIAdapter> adapter;
    hr = dxgi->GetAdapter(&adapter);
    if (FAILED(hr)) {
        return false;
    }
    hr = adapter->EnumOutputs(0, &output_);
    if (FAILED(hr)) {
        return false;
    }
    ComPtr<IDXGIOutput1> output1;
    hr = output_.As(&output1);
    if (FAILED(hr)) {
        return false;
    }
    hr = output1->DuplicateOutput(device_.Get(), &dup_);
    if (FAILED(hr)) {
        return false;
    }
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

ComPtr<ID3D11Texture2D> DxgiCapture::capture_frame()
{
    ComPtr<IDXGIResource> resource;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    HRESULT hr = dup_->AcquireNextFrame(INFINITE, &frame_info, &resource);
    if (FAILED(hr)) {
        return nullptr;
    }
    ComPtr<ID3D11Texture2D> frame;
    if (frame_info.TotalMetadataBufferSize != 0) {
        hr = resource.As(&frame);
        if (FAILED(hr)) {
            return nullptr;
        }
        return frame;
    }
    return nullptr;
}

void DxgiCapture::release_frame()
{
    dup_->ReleaseFrame();
}

std::vector<uint8_t> DxgiCapture::gpu_to_memory(ComPtr<ID3D11Texture2D> frame)
{
    D3D11_TEXTURE2D_DESC desc;
    frame->GetDesc(&desc);
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> tmp_frame;
    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, tmp_frame.GetAddressOf());
    if (FAILED(hr)) {
        return {};
    }
    context_->CopyResource(tmp_frame.Get(), frame.Get());

    D3D11_MAPPED_SUBRESOURCE  mapped;
    unsigned int subresource = 0;
    hr = context_->Map(tmp_frame.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        return {};
    }

    uint32_t width = desc.Width;
    uint32_t height = desc.Height;
    const int pitch = mapped.RowPitch;
    uint8_t* source = (uint8_t*)(mapped.pData);
    std::vector<uint8_t> dest;
    dest.resize(width * height * 4);
    uint8_t* dest_tmp = dest.data();
    for (int i = 0; i < height; ++i)
    {
        memcpy(dest_tmp, source, width * 4);
        source += pitch;
        dest_tmp += width * 4;
    }
    context_->Unmap(tmp_frame.Get(), 0);
    return dest;
}

}
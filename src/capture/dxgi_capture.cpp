#include <iostream>
#include "dxgi_capture.h"

using Microsoft::WRL::ComPtr;

namespace brtc {

bool DxgiCapture::init(ComPtr<ID3D11Device> device)
{
    device_ = device;
    device_->GetImmediateContext(context_.GetAddressOf());
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
        //return copy_frame(frame);
        return frame;
    }
    return nullptr;
}
void DxgiCapture::release_frame()
{
    dup_->ReleaseFrame();
}
ComPtr<ID3D11Texture2D> DxgiCapture::copy_frame(ComPtr<ID3D11Texture2D> inframe)
{
    D3D11_TEXTURE2D_DESC desc { 0 };
    //inframe->GetDesc(&desc);
    desc.Width = 1920;
    desc.Height = 1088; //?
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.MiscFlags = 0;
    desc.BindFlags = 0;
    ComPtr<ID3D11Texture2D> tmp_frame;
    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, tmp_frame.GetAddressOf());
    if (FAILED(hr)) {
        return {};
    }
    context_->CopyResource(tmp_frame.Get(), inframe.Get());
    return tmp_frame;
}
std::vector<uint8_t> DxgiCapture::gpu_to_memory(ComPtr<ID3D11Texture2D> frame)
{
    D3D11_TEXTURE2D_DESC desc;
    frame->GetDesc(&desc);
    //desc.MipLevels = 0;
    //desc.MiscFlags = 0;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> tmp_frame;
    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, tmp_frame.GetAddressOf());
    if (FAILED(hr)) {
        return {};
    }
    context_->CopyResource(tmp_frame.Get(), frame.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    unsigned int subresource = 0;
    hr = context_->Map(tmp_frame.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        return {};
    }

    uint32_t width = desc.Width;
    uint32_t height = desc.Height;
    const int pitch = mapped.RowPitch;
    uint8_t* source = (uint8_t*)(mapped.pData);
    //BYTE* dest = new BYTE[(width)*(height) * 4];
    std::vector<uint8_t> dest;
    dest.resize(width * height * 4);
    uint8_t* dest_tmp = dest.data();
    for (int i = 0; i < height; ++i) {
        memcpy(dest_tmp, source, width * 4);
        source += pitch;
        dest_tmp += width * 4;
    }
    context_->Unmap(tmp_frame.Get(), 0);
    return dest;
}


}
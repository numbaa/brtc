#include <iostream>
#include "d3d11_render.h"
#include <SDL_syswm.h>

namespace brtc {

namespace builtin {

using Microsoft::WRL::ComPtr;

bool D3D11Render::init(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    return init_sdl() && init_d3d11(device);
}

void D3D11Render::render_one_frame(Frame frame)
{
    ComPtr<ID3D11Texture2D> back_buffer;
    swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    context_->CopyResource(back_buffer.Get(), (ID3D11Texture2D*)frame.data);
    D3D11_BOX box { 0 };
    box.bottom = frame.height;
    box.right = frame.width;
    box.back = 1;
    context_->CopySubresourceRegion(back_buffer.Get(), 0, 0, 0, 0, (ID3D11Texture2D*)frame.data, 0, &box);
    //context_->CopyResource(back_buffer.Get(), frame);
    swap_chain_->Present(0, 0);
}


bool D3D11Render::init_sdl()
{
    SDL_Init(SDL_INIT_VIDEO);
    auto sdl = SDL_CreateWindow("brtc", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920 / 2, 1080 / 2, SDL_WINDOW_SHOWN);
    sdl_.reset(sdl);
    return sdl_ != nullptr;
}

bool D3D11Render::init_d3d11(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    device_ = device;
    device_->GetImmediateContext(context_.GetAddressOf());
    ComPtr<IDXGIDevice> dxgi_device;
    HRESULT hr = device_.As(&dxgi_device);
    if (FAILED(hr)) {
        return false;
    }
    ComPtr<IDXGIAdapter> dxgi_adapter;
    hr = dxgi_device->GetAdapter(&dxgi_adapter);
    if (FAILED(hr)) {
        return false;
    }
    ComPtr<IDXGIFactory1> dxgi_factory;
    hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgi_factory.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(sdl_.get(), &width, &height);
    SDL_SysWMinfo sys_info;
    SDL_VERSION(&sys_info.version);
    SDL_GetWindowWMInfo(sdl_.get(), &sys_info);

    DXGI_SWAP_CHAIN_DESC sd;
    ::memset(&sd, 0, sizeof(sd));
    sd.OutputWindow = sys_info.info.win.window;
    sd.BufferDesc.Width = 1920;
    sd.BufferDesc.Height = 1080;
    sd.BufferDesc.RefreshRate.Numerator = 30;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_NV12;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    hr = dxgi_factory->CreateSwapChain(device_.Get(), &sd, &swap_chain_);
    if (FAILED(hr)) {
        return false;
    }
    return true;
}

} // namespace builtin

} // namespace brtc

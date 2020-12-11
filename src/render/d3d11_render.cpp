#include <iostream>
#include "d3d11_render.h"
#include "../common/d3d_helper.h"

namespace brtc {

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

bool D3D11Render::init()
{
    return init_sdl() && init_d3d11();
}

void D3D11Render::on_frame(std::vector<uint8_t> frame)
{
    ComPtr<ID3D11Texture2D> back_buffer;
    swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    context_->UpdateSubresource(back_buffer.Get(), 0, nullptr, frame.data(), 1920 * 4/*TODO*/, 0);
    swap_chain_->Present(0, 0);
}

bool D3D11Render::init_sdl()
{
    SDL_Init(SDL_INIT_VIDEO);
    auto sdl = SDL_CreateWindow("brtc", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 400, SDL_WINDOW_SHOWN);
    sdl_.reset(sdl);
    return sdl_ != nullptr;
}

bool D3D11Render::init_d3d11()
{
    std::tie(device_, context_) = CreateD3D();
    if (!device_ || !context_) {
        return false;
    }
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
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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

}
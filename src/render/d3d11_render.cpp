#include <iostream>
#include "d3d11_render.h"

namespace brtc {



void D3D11Render::init()
{
    IDXGIDevice* dxgi_device = (IDXGIDevice*)d3d11_device_;
    IDXGIAdapter* dxgi_adapter;
    IDXGIFactory1* dxgi_factory;
    HRESULT hr = dxgi_device->GetAdapter(&dxgi_adapter);
    if (FAILED(hr)) {
        std::cerr << "get adapter failed\n";
        return;
    }
    hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgi_factory));
    if (FAILED(hr)) {
        std::cerr << "get parent failed\n";
        return;
    }
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = nullptr; //窗口句柄
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;
    hr = dxgi_factory->CreateSwapChain(d3d11_device_, &sd, &swap_chain_);
    if (FAILED(hr)) {
        std::cerr << "Create swap chain failed\n";
        return;
    }

}

//似乎不能传进来Texture2D？
void D3D11Render::on_frame(ID3D11Texture2D* frame)
{
    //todo 错误处理
    ID3D11Texture2D* back_buffer;
    void* _frame;
    swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    d3d11_context_->UpdateSubresource(back_buffer, 0, nullptr, _frame, 0/*?*/, 0);

}

}
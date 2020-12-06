#pragma once
#include <d3d11.h>

namespace brtc {

class D3D11Render {
public:
    void init();
    void on_frame(ID3D11Texture2D* frame);

private:
    ID3D11Device* d3d11_device_;
    ID3D11DeviceContext* d3d11_context_;
    IDXGISwapChain* swap_chain_;
};

}
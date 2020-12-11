#pragma once
#include <d3d11.h>
#include <vector>
#include <SDL.h>

namespace brtc {

class D3D11Render {
public:
    bool init();
    void on_frame(std::vector<uint8_t> frame);

private:
    bool init_sdl();
    bool init_d3d11();

private:
    struct SDLDeleter {
        void operator()(SDL_Window* win)
        {
            SDL_DestroyWindow(win);
        }
    };
    std::unique_ptr<SDL_Window, SDLDeleter> sdl_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
};

}
#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <memory>
#include "../common/video_source.h"

namespace brtc {

class DxgiCapture : public VideoSourceInterface {
public:
    bool init();
    void start() override;
    void stop() override;
    void add_observer(VideoSourceObserver* observer) override;

private:
    ID3D11Texture2D* get_frame();

private:
    //std::unique_ptr<IDXGIFactory1> dxgi_factory_;
    //std::unique_ptr<IDXGIDevice> dxgi_device_;
    //std::unique_ptr<ID3D11Device> d3d11_device_;
    //std::unique_ptr<ID3D11DeviceContext> d3d11_context_;
    ID3D11Device* d3d11_device_;
    ID3D11DeviceContext* d3d11_context_;
    IDXGIOutput* dxgi_output_;
    IDXGIOutputDuplication* dxgi_dup_;
    
};

}


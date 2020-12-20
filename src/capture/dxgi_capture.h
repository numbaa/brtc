#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include "../common/video_source.h"

namespace brtc {

class DxgiCapture : public VideoSourceInterface {
public:
    DxgiCapture() = default;
    bool init(Microsoft::WRL::ComPtr<ID3D11Device> device);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> capture_frame();
    void release_frame();
    Microsoft::WRL::ComPtr<ID3D11Texture2D> copy_frame(Microsoft::WRL::ComPtr<ID3D11Texture2D> inframe);
    std::vector<uint8_t> gpu_to_memory(Microsoft::WRL::ComPtr<ID3D11Texture2D> frame);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> dup_;
    Microsoft::WRL::ComPtr<IDXGIOutput> output_; //??
};

}


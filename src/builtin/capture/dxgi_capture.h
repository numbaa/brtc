#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

#include <brtc/frame.h>
#include <brtc/interface.h>

namespace brtc {

namespace builtin {

class DxgiCapture : public VideoCaptureInterface {
public:
    DxgiCapture() = default;
    Frame capture_one_frame() override;
    void release_frame() override;
    bool init(Microsoft::WRL::ComPtr<ID3D11Device> device);

private:
    Frame copy_frame(Microsoft::WRL::ComPtr<ID3D11Texture2D> inframe);
    std::vector<uint8_t> gpu_to_memory(Microsoft::WRL::ComPtr<ID3D11Texture2D> frame);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> dup_;
    Microsoft::WRL::ComPtr<IDXGIOutput> output_; //??
};

} // namespace builtin

} // namespace brtc



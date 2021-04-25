#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <memory>
#include <NvEncoder/NvEncoderD3D11.h>
#include <brtc/interface.h>

namespace brtc {

namespace builtin {

class NvEncoder : public VideoEncoderInterface {
public:
    NvEncoder() = default;
    ~NvEncoder() override;

    bool init(Microsoft::WRL::ComPtr<ID3D11Device> device, const NV_ENC_INITIALIZE_PARAMS& params);

    Frame encode_one_frame(Frame frame) override;

private:
    Frame encode_external_d3d11_texture2d(Frame frame);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    std::unique_ptr<NvEncoderD3D11> nvenc_;
};

} // namespace builtin

} // namespace brtc

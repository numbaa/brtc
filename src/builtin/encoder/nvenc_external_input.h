#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <cstdint>
#include <NvEncoder/NvEncoder.h>

namespace brtc {

namespace builtin {

class BrtcNvEncoder : public NvEncoder {
public:
    BrtcNvEncoder(Microsoft::WRL::ComPtr<ID3D11Device> device, uint32_t width, uint32_t height);
    void encode_external_d3d11_texture2d(void* frame, uint32_t width, uint32_t height);

private:
    void AllocateInputBuffers(int32_t) override { }
    void ReleaseInputBuffers() override { }

private:
    NV_ENC_REGISTER_RESOURCE registered_frame_ { NV_ENC_REGISTER_RESOURCE_VER };
};

} // namespace builtin

} // namespace brtc

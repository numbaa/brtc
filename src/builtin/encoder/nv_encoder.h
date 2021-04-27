#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <memory>
#include <NvEncoder/NvEncoder.h>
#include <brtc/interface.h>

namespace brtc {

namespace builtin {

class NvEncoder : public VideoEncoderInterface {
private:
    class ExternalFrameNvEncoder : public ::NvEncoder {
    public:
        ExternalFrameNvEncoder(Microsoft::WRL::ComPtr<ID3D11Device> device, uint32_t width, uint32_t height);
        void CreateEncoder(const NV_ENC_INITIALIZE_PARAMS* pEncodeParams);
        void encode_external_d3d11_texture2d(void* frame, uint32_t width, uint32_t height, std::vector<std::vector<uint8_t>>& packets);

    private:
        void AllocateInputBuffers(int32_t) override { }
        void ReleaseInputBuffers() override { }
        void try_update_input_buffers(void* frame, uint32_t width, uint32_t height);
        void get_encoded_frames(std::vector<NV_ENC_OUTPUT_PTR>& output_buffers, std::vector<std::vector<uint8_t>>& packets);
        NVENCSTATUS encode_next_frame();

    private:
        NV_ENC_REGISTER_RESOURCE registered_frame_ { NV_ENC_REGISTER_RESOURCE_VER };
        std::vector<NV_ENC_OUTPUT_PTR> output_buffer_;
    };

public:
    NvEncoder() = default;
    ~NvEncoder() override;

    bool init(Microsoft::WRL::ComPtr<ID3D11Device> device, const NV_ENC_INITIALIZE_PARAMS& params);

    Frame encode_one_frame(Frame frame) override;

private:
    Frame encode_external_d3d11_texture2d(Frame frame);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    std::unique_ptr<ExternalFrameNvEncoder> nvenc_;
};

} // namespace builtin

} // namespace brtc

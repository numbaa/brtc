#include "nvenc_external_input.h"

namespace brtc {

namespace builtin {

BrtcNvEncoder::BrtcNvEncoder(Microsoft::WRL::ComPtr<ID3D11Device> device, uint32_t width, uint32_t height)
    : NvEncoder(
        NV_ENC_DEVICE_TYPE::NV_ENC_DEVICE_TYPE_DIRECTX, device.Get(), width, height,
        NV_ENC_BUFFER_FORMAT::NV_ENC_BUFFER_FORMAT_ARGB, 0, false, false)
{
}

void BrtcNvEncoder::encode_external_d3d11_texture2d(void* frame, uint32_t width, uint32_t height)
{
    if (registered_frame_.resourceToRegister != nullptr && registered_frame_.resourceToRegister != frame) {
        NVENC_API_CALL(m_nvenc.nvEncUnregisterResource(m_hEncoder, registered_frame_.resourceToRegister));
        registered_frame_.resourceToRegister = nullptr;
    }
    if (registered_frame_.resourceToRegister == nullptr) {
        registered_frame_.resourceToRegister = frame;
        registered_frame_.bufferUsage = NV_ENC_BUFFER_USAGE::NV_ENC_INPUT_IMAGE;
        registered_frame_.bufferFormat = NV_ENC_BUFFER_FORMAT::NV_ENC_BUFFER_FORMAT_ARGB;
        registered_frame_.height = height;
        registered_frame_.pitch = 0;
        registered_frame_.width = width;
        registered_frame_.resourceType = NV_ENC_INPUT_RESOURCE_TYPE::NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
        NVENC_API_CALL(m_nvenc.nvEncRegisterResource(m_hEncoder, &registered_frame_));
    }

    NV_ENC_MAP_INPUT_RESOURCE map_input_resource = { NV_ENC_MAP_INPUT_RESOURCE_VER };
    map_input_resource.registeredResource = registered_frame_.registeredResource;
    NVENC_API_CALL(m_nvenc.nvEncMapInputResource(m_hEncoder, &map_input_resource));

    int bfrIdx = m_iToSend % m_nEncoderBuffer;
    m_vMappedInputBuffers[bfrIdx] = map_input_resource.mappedResource;
    DoEncode(m_vMappedInputBuffers[bfrIdx], m_vBitstreamOutputBuffer[bfrIdx], nullptr);
}

} // namespace builtin

} // namespace brtc

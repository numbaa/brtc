#include <memory>
#include "common/time_utils.h"
#include "nv_encoder.h"
#include <NvEncoderCLIOptions.h>
#include <Logger.h>

simplelogger::Logger* logger;

namespace brtc {

namespace builtin {

bool NvEncoder::init(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    logger = simplelogger::LoggerFactory::CreateConsoleLogger();
    device_ = device;
    auto nvenc = std::make_unique<NvEncoder::ExternalFrameNvEncoder>(device_.Get(), 1920, 1080);

    NvEncoderInitParam cli_opts { "-profile main -codec h264 -fps 30 -bitrate 4194304 -maxbitrate 8388608 -preset p1 -tuninginfo lowlatency" };
    NV_ENC_INITIALIZE_PARAMS init_params = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG enc_cfg = { NV_ENC_CONFIG_VER };
    init_params.encodeConfig = &enc_cfg;
    nvenc->CreateDefaultEncoderParams(&init_params, cli_opts.GetEncodeGUID(), cli_opts.GetPresetGUID(), cli_opts.GetTuningInfo());
    cli_opts.SetInitParams(&init_params, NV_ENC_BUFFER_FORMAT_ARGB);
    try {
        nvenc->CreateEncoder(&init_params);
    } catch (const NVENCException& e) {
        std::cout << e.getErrorCode() << ": " << e.getErrorString() << std::endl;
        return false;
    }
    nvenc_ = std::move(nvenc);
    return true;
}

Frame NvEncoder::encode_one_frame(Frame frame)
{
    switch (frame.type) {
    case brtc::Frame::UnderlyingType::kMemory:
        break;
    case brtc::Frame::UnderlyingType::kD3D11Texture2D:
        return encode_external_d3d11_texture2d(frame);
    default:
        break;
    }
    return Frame();
}

Frame NvEncoder::encode_external_d3d11_texture2d(Frame frame)
{
    auto packets = std::make_shared<std::vector<std::vector<uint8_t>>>();
    nvenc_->encode_external_d3d11_texture2d(frame.data, frame.width, frame.height, *packets);
    Frame out;
    if (!packets->empty()) {
        out.length = packets->front().size();
        out.height = frame.height;
        out.width = frame.width;
        out.data = packets->front().data();
        out.type = Frame::UnderlyingType::kMemory;
        out.timestamp = static_cast<uint32_t>(brtc::MachineNowMilliseconds());
        out._data_holder = packets;
    }
    return out;
}

NvEncoder::ExternalFrameNvEncoder::ExternalFrameNvEncoder(Microsoft::WRL::ComPtr<ID3D11Device> device, uint32_t width, uint32_t height)
    : NvEncoder(
        NV_ENC_DEVICE_TYPE::NV_ENC_DEVICE_TYPE_DIRECTX, device.Get(), width, height,
        NV_ENC_BUFFER_FORMAT::NV_ENC_BUFFER_FORMAT_ARGB, 0, false, false)
{
}

void NvEncoder::ExternalFrameNvEncoder::encode_external_d3d11_texture2d(void* frame, uint32_t width, uint32_t height, std::vector<std::vector<uint8_t>>& packets)
{
    try_update_input_buffers(frame, width, height);

    NVENCSTATUS status = encode_next_frame();

    if (status == NV_ENC_SUCCESS || status == NV_ENC_ERR_NEED_MORE_INPUT) {
        m_iToSend++;
        get_encoded_frames(output_buffer_, packets);
        assert(packets.size() == 0 || packets.size() == 1);
    } else {
        NVENC_THROW_ERROR("nvEncEncodePicture API failed", status);
    }
}

void NvEncoder::ExternalFrameNvEncoder::AllocateInputBuffers(int32_t size)
{
    output_buffer_.resize(size);
    for (int i = 0; i < size; i++) {
        NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBuffer = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER };
        NVENC_API_CALL(m_nvenc.nvEncCreateBitstreamBuffer(m_hEncoder, &createBitstreamBuffer));
        output_buffer_[i] = createBitstreamBuffer.bitstreamBuffer;
    }
}

void NvEncoder::ExternalFrameNvEncoder::try_update_input_buffers(void* frame, uint32_t width, uint32_t height)
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
}

void NvEncoder::ExternalFrameNvEncoder::get_encoded_frames(std::vector<NV_ENC_OUTPUT_PTR>& output_buffers, std::vector<std::vector<uint8_t>>& packets)
{
    unsigned i = 0;
    for (; m_iGot < m_iToSend; m_iGot++) {
        WaitForCompletionEvent(m_iGot % m_nEncoderBuffer);
        NV_ENC_LOCK_BITSTREAM lockBitstreamData = { NV_ENC_LOCK_BITSTREAM_VER };
        lockBitstreamData.outputBitstream = output_buffers[m_iGot % m_nEncoderBuffer];
        lockBitstreamData.doNotWait = false;
        NVENC_API_CALL(m_nvenc.nvEncLockBitstream(m_hEncoder, &lockBitstreamData));

        uint8_t* pData = (uint8_t*)lockBitstreamData.bitstreamBufferPtr;
        if (packets.size() < i + 1) {
            packets.push_back(std::vector<uint8_t>());
        }
        packets[i].clear();
        packets[i].insert(packets[i].end(), &pData[0], &pData[lockBitstreamData.bitstreamSizeInBytes]);
        i++;

        NVENC_API_CALL(m_nvenc.nvEncUnlockBitstream(m_hEncoder, lockBitstreamData.outputBitstream));

        if (m_vMappedInputBuffers[m_iGot % m_nEncoderBuffer]) {
            NVENC_API_CALL(m_nvenc.nvEncUnmapInputResource(m_hEncoder, m_vMappedInputBuffers[m_iGot % m_nEncoderBuffer]));
            m_vMappedInputBuffers[m_iGot % m_nEncoderBuffer] = nullptr;
        }
    }
}

NVENCSTATUS NvEncoder::ExternalFrameNvEncoder::encode_next_frame()
{
    NV_ENC_MAP_INPUT_RESOURCE map_input_resource = { NV_ENC_MAP_INPUT_RESOURCE_VER };
    map_input_resource.registeredResource = registered_frame_.registeredResource;
    NVENC_API_CALL(m_nvenc.nvEncMapInputResource(m_hEncoder, &map_input_resource));

    int bfrIdx = m_iToSend % m_nEncoderBuffer;
    m_vMappedInputBuffers[bfrIdx] = map_input_resource.mappedResource;
    return DoEncode(m_vMappedInputBuffers[bfrIdx], output_buffer_[bfrIdx], nullptr);
}

} // namespace builtin

} // namespace brtc

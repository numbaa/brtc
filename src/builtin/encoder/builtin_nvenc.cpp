#include <NvEncoderCLIOptions.h>
#include "builtin_nvenc.h"

namespace brtc {

namespace builtin {

bool NvEncoder::init(Microsoft::WRL::ComPtr<ID3D11Device> device, const NV_ENC_INITIALIZE_PARAMS& params)
{
    device_ = device;
    nvenc_ = std::make_unique<NvEncoderD3D11>(device_.Get(), 1920, 1080, NV_ENC_BUFFER_FORMAT_ARGB);

    NvEncoderInitParam cli_opts;
    NV_ENC_INITIALIZE_PARAMS init_params = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG enc_cfg = { NV_ENC_CONFIG_VER };
    init_params.encodeConfig = &enc_cfg;
    nvenc_->CreateDefaultEncoderParams(&init_params, cli_opts.GetEncodeGUID(), cli_opts.GetPresetGUID());
    cli_opts.SetInitParams(&init_params, NV_ENC_BUFFER_FORMAT_ARGB);
    nvenc_->CreateEncoder(&init_params);
}

Frame NvEncoder::encode_one_frame(Frame frame)
{
    const NvEncInputFrame* encoderInputFrame = nvenc_->GetNextInputFrame();
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
    //register
    //encode
    //unregister
    return Frame();
}

} // namespace builtin

} // namespace brtc

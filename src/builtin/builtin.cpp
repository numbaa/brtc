#include <brtc/builtin.h>

#include "builtin/capture/dxgi_capture.h"
#include "builtin/decoder/mfx_decoder.h"
#include "builtin/encoder/mfx_encoder.h"
#include "builtin/render/d3d11_render.h"

namespace brtc {

namespace builtin {

std::unique_ptr<VideoCaptureInterface> create_capture_dxgi(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    std::unique_ptr<DxgiCapture> capture { new DxgiCapture };
    if (!capture->init(device)) {
        return nullptr;
    } else {
        return std::move(capture);
    }
}

std::unique_ptr<VideoEncoderInterface> create_encoder_mfx(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    std::unique_ptr<MfxEncoder> encoder { new MfxEncoder };
    if (!encoder->init(device)) {
        return nullptr;
    } else {
        return std::move(encoder);
    }
}

std::unique_ptr<VideoDecoderInterface> create_decoder_mfx(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    std::unique_ptr<MfxDecoder> decoder { new MfxDecoder };
    if (!decoder->init(device)) {
        return nullptr;
    } else {
        return std::move(decoder);
    }
}

std::unique_ptr<RenderInterface> create_render_d3d11(Microsoft::WRL::ComPtr<ID3D11Device> device)
{
    std::unique_ptr<D3D11Render> render { new D3D11Render };
    if (!render->init(device)) {
        return nullptr;
    } else {
        return std::move(render);
    }
}

} // namespace builtin

} // namespace brtc

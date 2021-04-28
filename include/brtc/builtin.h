#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <memory>
#include <brtc.h>

namespace brtc {

namespace builtin {

std::unique_ptr<VideoCaptureInterface> create_capture_dxgi(Microsoft::WRL::ComPtr<ID3D11Device> device);

std::unique_ptr<VideoEncoderInterface> create_encoder_mfx(Microsoft::WRL::ComPtr<ID3D11Device> device);

std::unique_ptr<VideoEncoderInterface> create_encoder_nv(Microsoft::WRL::ComPtr<ID3D11Device> device);

std::unique_ptr<VideoDecoderInterface> create_decoder_mfx(Microsoft::WRL::ComPtr<ID3D11Device> device);

std::unique_ptr<RenderInterface> create_render_d3d11(Microsoft::WRL::ComPtr<ID3D11Device> device);

std::unique_ptr<Strategies> create_strategies(int which);

} //namespace builtin

} // namespace brtc

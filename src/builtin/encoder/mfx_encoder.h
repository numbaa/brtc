#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <map>
#include <vector>
#include <memory>

#include <mfxvideo.h>

#include <brtc/interface.h>

#include "common/mfx_allocator.h"

namespace brtc {

class MfxEncoder : public VideoEncoderInterface {
public:
    MfxEncoder() = default;
    ~MfxEncoder();

    Frame encode_one_frame(Frame frame) override;

    bool init(Microsoft::WRL::ComPtr<ID3D11Device> device);

private:
    bool init_encoder();
    bool init_vpp();
    Microsoft::WRL::ComPtr<ID3D11Texture2D> alloc_render_surface();

    mfxVideoParam gen_encode_param();
    mfxVideoParam gen_vpp_param();

private:
    std::shared_ptr<MfxFrameAllocator> allocator_;
    mfxVideoParam encode_param_;
    mfxVideoParam vpp_param_;
    mfxSession mfx_session_ = nullptr;
    Microsoft::WRL::ComPtr<IDXGIFactory2> factory_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> render_surface_;
};

}
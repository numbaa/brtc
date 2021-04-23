#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <array>
#include <vector>
#include <map>

#include <brtc/interface.h>

#include "../mfx_common/mfx_allocator.h"

#ifdef _WIN32
#define LD3D_CDECL __cdecl
#else
#define LD3D_CDECL
#endif

namespace brtc {

namespace builtin {

class MfxDecoder : public VideoDecoderInterface {
public:
    MfxDecoder() = default;
    Frame decode_one_frame(Frame encoded_frame) override;
    bool init(Microsoft::WRL::ComPtr<ID3D11Device> device);
    int init2(Frame encoded_frame);

    mfxStatus release_frame(Microsoft::WRL::ComPtr<ID3D11Texture2D> frame);

private:
    int32_t get_unlocked_frame();

private:
    bool already_init_ = false;
    std::shared_ptr<MfxDecoderFrameAllocator> allocator_;
    std::vector<mfxFrameSurface1> surfaces_;
    mfxVideoParam decode_param_;
    mfxSession mfx_session_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
};

} // namespace builtin

} // namespace brtc

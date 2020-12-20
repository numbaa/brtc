#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <array>
#include <vector>
#include <map>

#include "../common/mfx_allocator.h"

#ifdef _WIN32
#define LD3D_CDECL __cdecl
#else
#define LD3D_CDECL
#endif

namespace brtc {

class MfxDecoder {
public:
    MfxDecoder() = default;
    bool init(Microsoft::WRL::ComPtr<ID3D11Device> device);
    int init2(std::vector<uint8_t> encoded_frame);
    ID3D11Texture2D* on_frame(std::vector<uint8_t>& encoded_frame);
    mfxStatus release_frame(Microsoft::WRL::ComPtr<ID3D11Texture2D> frame);

private:
    int32_t get_unlocked_frame();

private:
    bool already_init_ = false;
    std::shared_ptr<MfxFrameAllocator> allocator_;
    std::vector<mfxFrameSurface1> surfaces_;
    mfxVideoParam decode_param_;
    mfxSession mfx_session_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
};

}
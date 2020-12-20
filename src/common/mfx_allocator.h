#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include <vector>
#include <map>

#include <mfxvideo.h>

namespace brtc {

//TODO: auto manage mids
struct FrameBuffer {
    mfxMemId* mids = nullptr;
    std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>> frames;
    ~FrameBuffer()
    {
        if (mids)
            delete mids;
    }
};

class MfxFrameAllocator : public mfxFrameAllocator {
public:
    MfxFrameAllocator();
    virtual ~MfxFrameAllocator() = default;
    virtual mfxStatus alloc(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) = 0;
    virtual mfxStatus lock(mfxMemId mid, mfxFrameData* ptr) = 0;
    virtual mfxStatus unlock(mfxMemId mid, mfxFrameData* ptr) = 0;
    virtual mfxStatus get_hdl(mfxMemId mid, mfxHDL* handle) = 0;
    virtual mfxStatus free(mfxFrameAllocResponse* response) = 0;

private:
    static mfxStatus MFX_CDECL _alloc(mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response);
    static mfxStatus MFX_CDECL _lock(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr);
    static mfxStatus MFX_CDECL _unlock(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr);
    static mfxStatus MFX_CDECL _getHDL(mfxHDL pthis, mfxMemId mid, mfxHDL* handle);
    static mfxStatus MFX_CDECL _free(mfxHDL pthis, mfxFrameAllocResponse* response);
};

#ifdef  _WIN32
class D3D11FrameAllocator : public MfxFrameAllocator {
public:
    D3D11FrameAllocator(Microsoft::WRL::ComPtr<ID3D11Device> device);
    mfxStatus alloc(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) override;
    mfxStatus lock(mfxMemId mid, mfxFrameData* ptr) override;
    mfxStatus unlock(mfxMemId mid, mfxFrameData* ptr) override;
    mfxStatus get_hdl(mfxMemId mid, mfxHDL* handle) override;
    mfxStatus free(mfxFrameAllocResponse* response) override;

private:
    mfxStatus alloc_external_frame(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response);
    mfxStatus alloc_internal_frame(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response);

private:
    FrameBuffer external_frames_;
    std::map<mfxMemId*, FrameBuffer> internal_frames_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
};
#endif //  _WIN32


}
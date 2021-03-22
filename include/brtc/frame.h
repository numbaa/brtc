#pragma once
#include <cstdint>

namespace brtc {

struct Frame {
    enum class UnderlyingType : uint32_t {
        kUknown,
        kMemory,
        kD3D9Surface,
        kD3D11Texture2D,
        kOpenGLTexture2D,
    };
    void* data;
    uint32_t width;
    uint32_t height;
};

} // brtc

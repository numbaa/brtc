#pragma once
#include <cstdint>
#include <any>
#include <optional>

namespace brtc {

struct Frame {
    enum class UnderlyingType : uint32_t {
        kUknown,
        kMemory,
        kD3D9Surface,
        kD3D11Texture2D,
        kOpenGLTexture2D,
    };
    UnderlyingType underlying_type = UnderlyingType::kUknown;
    void* data = nullptr;
    uint32_t length = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t timestamp = 0; // ??
    std::any _data_holder;
};

} // brtc

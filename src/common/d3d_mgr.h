#pragma once
#include <d3d11.h>
#include <wrl/client.h>

namespace brtc {

class D3DMgr {
public:
    enum class GpuVender {
        Intel,
        Nvidia,
        Amd,
    };
    D3DMgr() = default;
    bool init(GpuVender vender);
    Microsoft::WRL::ComPtr<ID3D11Device> get_device();

private:
    bool create_device(Microsoft::WRL::ComPtr<IDXGIAdapter> adapter);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
};

}
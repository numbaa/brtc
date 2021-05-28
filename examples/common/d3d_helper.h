#pragma once
#include <wrl/client.h>
#include <d3d11.h>

enum class GpuVendor {
    Intel,
    Amd,
    Nvidia,
};

Microsoft::WRL::ComPtr<ID3D11Device> get_d3d11_device(GpuVendor vendor);
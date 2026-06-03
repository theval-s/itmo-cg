//
// D3D11 backend — deduplicating cache for fixed-function state objects.
//
// Rasterizer / blend / depth-stencil / sampler states are a tiny immutable set.
// Components describe the state they want (via *Desc); the cache returns a shared
// object, so identical states are created exactly once.
//
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include "../rhi_descs.hpp"

namespace val_cg::rhi::d3d11 {

    using Microsoft::WRL::ComPtr;

    class D3D11StateCache {
    public:
        explicit D3D11StateCache(ID3D11Device* device) : device(device) {}

        ID3D11RasterizerState*   GetRasterizer(const RasterizerDesc& desc);
        ID3D11BlendState*        GetBlend(const BlendDesc& desc);
        ID3D11DepthStencilState* GetDepthStencil(const DepthStencilDesc& desc);
        ID3D11SamplerState*      GetSampler(const SamplerDesc& desc);

    private:
        template <class Desc, class State>
        struct Entry { Desc desc; ComPtr<State> state; };

        ID3D11Device* device = nullptr;
        std::vector<Entry<RasterizerDesc,   ID3D11RasterizerState>>   rasterizers;
        std::vector<Entry<BlendDesc,        ID3D11BlendState>>        blends;
        std::vector<Entry<DepthStencilDesc, ID3D11DepthStencilState>> depthStencils;
        std::vector<Entry<SamplerDesc,      ID3D11SamplerState>>      samplers;
    };

}

//
// D3D11 backend — state cache implementation.
//
#include "d3d11_state_cache.hpp"
#include "d3d11_translate.hpp"

namespace val_cg::rhi::d3d11 {

    ID3D11RasterizerState* D3D11StateCache::GetRasterizer(const RasterizerDesc& desc) {
        for (auto& e : rasterizers)
            if (e.desc == desc) return e.state.Get();

        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode              = ToD3D(desc.fill);
        rd.CullMode              = ToD3D(desc.cull);
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable       = desc.depthClip ? TRUE : FALSE;
        rd.DepthBias             = desc.depthBias;
        rd.SlopeScaledDepthBias  = desc.slopeScaledDepthBias;

        ComPtr<ID3D11RasterizerState> state;
        device->CreateRasterizerState(&rd, &state);
        rasterizers.push_back({desc, state});
        return state.Get();
    }

    ID3D11BlendState* D3D11StateCache::GetBlend(const BlendDesc& desc) {
        for (auto& e : blends)
            if (e.desc == desc) return e.state.Get();

        D3D11_BLEND_DESC bd = {};
        auto& rt = bd.RenderTarget[0];
        rt.BlendEnable    = desc.enable ? TRUE : FALSE;
        rt.SrcBlend       = ToD3D(desc.srcColor);
        rt.DestBlend      = ToD3D(desc.dstColor);
        rt.BlendOp        = ToD3D(desc.opColor);
        rt.SrcBlendAlpha  = ToD3D(desc.srcAlpha);
        rt.DestBlendAlpha = ToD3D(desc.dstAlpha);
        rt.BlendOpAlpha   = ToD3D(desc.opAlpha);
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        ComPtr<ID3D11BlendState> state;
        device->CreateBlendState(&bd, &state);
        blends.push_back({desc, state});
        return state.Get();
    }

    ID3D11DepthStencilState* D3D11StateCache::GetDepthStencil(const DepthStencilDesc& desc) {
        for (auto& e : depthStencils)
            if (e.desc == desc) return e.state.Get();

        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable    = desc.depthTest ? TRUE : FALSE;
        dd.DepthWriteMask = desc.depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL
                                            : D3D11_DEPTH_WRITE_MASK_ZERO;
        dd.DepthFunc      = ToD3D(desc.func);
        dd.StencilEnable  = FALSE;

        ComPtr<ID3D11DepthStencilState> state;
        device->CreateDepthStencilState(&dd, &state);
        depthStencils.push_back({desc, state});
        return state.Get();
    }

    ID3D11SamplerState* D3D11StateCache::GetSampler(const SamplerDesc& desc) {
        for (auto& e : samplers)
            if (e.desc == desc) return e.state.Get();

        D3D11_SAMPLER_DESC sd = {};
        switch (desc.filter) {
            case Filter::Point:      sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; break;
            case Filter::Linear:     sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; break;
            case Filter::Comparison: sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; break;
        }
        sd.AddressU = sd.AddressV = sd.AddressW = ToD3D(desc.address);
        sd.ComparisonFunc = (desc.filter == Filter::Comparison) ? ToD3D(desc.compare)
                                                                : D3D11_COMPARISON_NEVER;
        sd.BorderColor[0] = sd.BorderColor[1] =
        sd.BorderColor[2] = sd.BorderColor[3] = desc.borderColor;
        sd.MaxLOD = D3D11_FLOAT32_MAX;

        ComPtr<ID3D11SamplerState> state;
        device->CreateSamplerState(&sd, &state);
        samplers.push_back({desc, state});
        return state.Get();
    }

}

//
// D3D11 backend — concrete RHI resource implementations.
//
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <cstring>
#include "../rhi_resources.hpp"
#include "../rhi_enums.hpp"

namespace val_cg::rhi::d3d11 {

    using Microsoft::WRL::ComPtr;

    class D3D11Shader : public GpuShader {
    public:
        ShaderStage              stage{};
        ComPtr<ID3D11VertexShader> vs;   // valid when stage == Vertex
        ComPtr<ID3D11PixelShader>  ps;   // valid when stage == Pixel
        ComPtr<ID3DBlob>           bytecode;  // kept only for input-layout creation at pipeline build
    };

    class D3D11Buffer : public GpuBuffer {
    public:
        ComPtr<ID3D11Buffer>     buffer;
        ID3D11DeviceContext*     ctx     = nullptr;  // non-owning
        bool                     dynamic = false;

        void Update(const void* data, size_t bytes) override {
            if (!buffer || !ctx) return;
            if (dynamic) {
                D3D11_MAPPED_SUBRESOURCE mapped = {};
                ctx->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memcpy(mapped.pData, data, bytes);
                ctx->Unmap(buffer.Get(), 0);
            } else {
                ctx->UpdateSubresource(buffer.Get(), 0, nullptr, data, 0, 0);
            }
        }
    };

    // States (rs/bs/dss) are owned by the device's state cache; the pipeline only
    // references them. It owns the input layout it builds.
    class D3D11Pipeline : public GpuPipeline {
    public:
        ID3D11VertexShader*      vs   = nullptr;  // non-owning (owned by cached D3D11Shader)
        ID3D11PixelShader*       ps   = nullptr;
        ComPtr<ID3D11InputLayout> layout;
        ID3D11RasterizerState*   rs   = nullptr;  // non-owning (state cache)
        ID3D11BlendState*        bs   = nullptr;  // non-owning (state cache)
        ID3D11DepthStencilState* dss  = nullptr;  // non-owning (state cache)
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    };

    class D3D11Texture : public GpuTexture {
    public:
        ComPtr<ID3D11Texture2D>          texture;
        ComPtr<ID3D11ShaderResourceView> srv;
    };

    class D3D11RenderTarget : public GpuRenderTarget {
    public:
        ComPtr<ID3D11RenderTargetView> rtv;
    };

    class D3D11DepthTarget : public GpuDepthTarget {
    public:
        ComPtr<ID3D11DepthStencilView> dsv;
    };

    class D3D11Sampler : public GpuSampler {
    public:
        ComPtr<ID3D11SamplerState> sampler;
    };

}

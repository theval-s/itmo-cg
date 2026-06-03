//
// D3D11 backend — CommandList implementation.
//
#include "d3d11_command_list.hpp"
#include "d3d11_resources.hpp"
#include "d3d11_translate.hpp"
#include <vector>

namespace val_cg::rhi::d3d11 {

    void D3D11CommandList::SetRenderTargets(GpuRenderTarget* const* targets, int count,
                                            GpuDepthTarget* depth) {
        ID3D11RenderTargetView* rtvs[8] = {};
        if (count > 8) count = 8;
        for (int i = 0; i < count; ++i)
            rtvs[i] = targets[i] ? static_cast<D3D11RenderTarget*>(targets[i])->rtv.Get() : nullptr;
        auto* dsv = depth ? static_cast<D3D11DepthTarget*>(depth)->dsv.Get() : nullptr;
        ctx->OMSetRenderTargets(static_cast<UINT>(count), count ? rtvs : nullptr, dsv);
    }

    void D3D11CommandList::ClearRenderTarget(GpuRenderTarget* target, const float rgba[4]) {
        if (target)
            ctx->ClearRenderTargetView(static_cast<D3D11RenderTarget*>(target)->rtv.Get(), rgba);
    }

    void D3D11CommandList::ClearDepth(GpuDepthTarget* depth, float value) {
        if (depth)
            ctx->ClearDepthStencilView(static_cast<D3D11DepthTarget*>(depth)->dsv.Get(),
                                       D3D11_CLEAR_DEPTH, value, 0);
    }

    void D3D11CommandList::SetViewport(float x, float y, float width, float height) {
        D3D11_VIEWPORT vp = {};
        vp.TopLeftX = x;
        vp.TopLeftY = y;
        vp.Width    = width;
        vp.Height   = height;
        vp.MinDepth = 0.f;
        vp.MaxDepth = 1.f;
        ctx->RSSetViewports(1, &vp);
    }

    void D3D11CommandList::SetPipeline(GpuPipeline* pipeline) {
        if (!pipeline) return;
        auto* p = static_cast<D3D11Pipeline*>(pipeline);
        ctx->VSSetShader(p->vs, nullptr, 0);
        ctx->PSSetShader(p->ps, nullptr, 0);
        ctx->IASetInputLayout(p->layout.Get());
        ctx->IASetPrimitiveTopology(p->topology);
        ctx->RSSetState(p->rs);
        const float blendFactor[4] = {0, 0, 0, 0};
        ctx->OMSetBlendState(p->bs, blendFactor, 0xffffffff);
        ctx->OMSetDepthStencilState(p->dss, 0);
    }

    void D3D11CommandList::SetVertexBuffer(GpuBuffer* vb, unsigned stride) {
        ID3D11Buffer* buf = vb ? static_cast<D3D11Buffer*>(vb)->buffer.Get() : nullptr;
        UINT s = stride, o = 0;
        ctx->IASetVertexBuffers(0, 1, &buf, &s, &o);
    }

    void D3D11CommandList::SetIndexBuffer(GpuBuffer* ib, IndexFormat format) {
        ID3D11Buffer* buf = ib ? static_cast<D3D11Buffer*>(ib)->buffer.Get() : nullptr;
        ctx->IASetIndexBuffer(buf, ToDXGI(format), 0);
    }

    void D3D11CommandList::SetConstantBuffer(ShaderStage stage, unsigned slot, GpuBuffer* cb) {
        ID3D11Buffer* buf = cb ? static_cast<D3D11Buffer*>(cb)->buffer.Get() : nullptr;
        if (stage == ShaderStage::Vertex) ctx->VSSetConstantBuffers(slot, 1, &buf);
        else                              ctx->PSSetConstantBuffers(slot, 1, &buf);
    }

    void D3D11CommandList::SetTexture(ShaderStage stage, unsigned slot, GpuTexture* tex) {
        ID3D11ShaderResourceView* srv = tex ? static_cast<D3D11Texture*>(tex)->srv.Get() : nullptr;
        if (stage == ShaderStage::Vertex) ctx->VSSetShaderResources(slot, 1, &srv);
        else                              ctx->PSSetShaderResources(slot, 1, &srv);
    }

    void D3D11CommandList::SetSampler(ShaderStage stage, unsigned slot, GpuSampler* samp) {
        ID3D11SamplerState* s = samp ? static_cast<D3D11Sampler*>(samp)->sampler.Get() : nullptr;
        if (stage == ShaderStage::Vertex) ctx->VSSetSamplers(slot, 1, &s);
        else                              ctx->PSSetSamplers(slot, 1, &s);
    }

    void D3D11CommandList::Draw(unsigned vertexCount, unsigned startVertex) {
        ctx->Draw(vertexCount, startVertex);
    }

    void D3D11CommandList::DrawIndexed(unsigned indexCount, unsigned startIndex, int baseVertex) {
        ctx->DrawIndexed(indexCount, startIndex, baseVertex);
    }

    void D3D11CommandList::UnbindTextures(ShaderStage stage, unsigned slot, int count) {
        ID3D11ShaderResourceView* nulls[16] = {};
        if (count > 16) count = 16;
        if (stage == ShaderStage::Vertex) ctx->VSSetShaderResources(slot, count, nulls);
        else                              ctx->PSSetShaderResources(slot, count, nulls);
    }

}

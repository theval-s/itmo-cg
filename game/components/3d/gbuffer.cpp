#include "gbuffer.hpp"
#include <stdexcept>

namespace val_cg {

    void GBuffer::Initialize(ID3D11Device* device, int width, int height) {
        auto makeRT = [&](DXGI_FORMAT fmt,
                          ID3D11Texture2D** tex,
                          ID3D11RenderTargetView** rtv,
                          ID3D11ShaderResourceView** srv)
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width     = width;
            desc.Height    = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format    = fmt;
            desc.SampleDesc.Count = 1;
            desc.Usage     = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            if (FAILED(device->CreateTexture2D(&desc, nullptr, tex)))
                throw std::runtime_error("GBuffer: failed to create texture");

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format        = fmt;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            if (FAILED(device->CreateRenderTargetView(*tex, &rtvDesc, rtv)))
                throw std::runtime_error("GBuffer: failed to create RTV");

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                    = fmt;
            srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels       = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            if (FAILED(device->CreateShaderResourceView(*tex, &srvDesc, srv)))
                throw std::runtime_error("GBuffer: failed to create SRV");
        };

        makeRT(DXGI_FORMAT_R8G8B8A8_UNORM,         &tex0, &rtv0, &srv0);  // DiffuseSpec
        makeRT(DXGI_FORMAT_R16G16B16A16_FLOAT,      &tex1, &rtv1, &srv1);  // Normal
    }

    void GBuffer::DestroyResources() {
        if (rtv0) { rtv0->Release(); rtv0 = nullptr; }
        if (rtv1) { rtv1->Release(); rtv1 = nullptr; }
        if (srv0) { srv0->Release(); srv0 = nullptr; }
        if (srv1) { srv1->Release(); srv1 = nullptr; }
        if (tex0) { tex0->Release(); tex0 = nullptr; }
        if (tex1) { tex1->Release(); tex1 = nullptr; }
    }

    void GBuffer::Clear(ID3D11DeviceContext* ctx) {
        const float black[4] = {0.f, 0.f, 0.f, 0.f};
        ctx->ClearRenderTargetView(rtv0, black);
        ctx->ClearRenderTargetView(rtv1, black);
    }

    void GBuffer::Bind(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv) {
        ID3D11RenderTargetView* rtvs[2] = {rtv0, rtv1};
        ctx->OMSetRenderTargets(2, rtvs, dsv);
    }

    void GBuffer::BindSRVs(ID3D11DeviceContext* ctx, int startSlot) {
        ID3D11ShaderResourceView* srvs[2] = {srv0, srv1};
        ctx->PSSetShaderResources(startSlot, 2, srvs);
    }

    void GBuffer::UnbindSRVs(ID3D11DeviceContext* ctx, int startSlot) {
        ID3D11ShaderResourceView* nulls[2] = {nullptr, nullptr};
        ctx->PSSetShaderResources(startSlot, 2, nulls);
    }
}

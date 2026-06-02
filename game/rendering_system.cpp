#include "rendering_system.hpp"
#include "game.hpp"
#include "consts.hpp"
#include "components/3d/lit_mesh_component.hpp"
#include "components/3d/katamari_player_component.hpp"
#include "components/3d/shadow_map_component.hpp"
#include "components/3d/lights/spot_light_component.hpp"
#include <d3dcompiler.h>
#include <iostream>
#include <stdexcept>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    RenderingSystem::RenderingSystem(Game* g) : game(g) {}

    // -------------------------------------------------------------------------
    void RenderingSystem::CompileShader(const wchar_t* path, const char* entry,
                                        const char* target, ID3DBlob** outBlob) {
        ID3DBlob* error = nullptr;
        HRESULT hr = D3DCompileFromFile(path, nullptr, nullptr, entry, target,
                                        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                                        0, outBlob, &error);
        if (FAILED(hr)) {
            if (error) {
                std::cerr << "[RenderingSystem] Shader error in " << entry << ": "
                          << (char*)error->GetBufferPointer() << "\n";
                error->Release();
            }
            throw std::runtime_error("RenderingSystem: shader compilation failed");
        }
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::Initialize() {
        auto* dev = game->renderer.device.Get();
        const int w = game->renderer.ScreenWidth;
        const int h = game->renderer.ScreenHeight;

        gbuffer.Initialize(dev, w, h);

        // ---- Geometry pass shaders ----
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;

        // Non-textured & textured lit meshes share the same VS (POSITION+NORMAL).
        CompileShader(GBUFFER_SHADER_PATH, "VSMain", "vs_5_0", &vsBlob);
        CompileShader(GBUFFER_SHADER_PATH, "PSMain", "ps_5_0", &psBlob);
        dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &gbufferVS);
        dev->CreatePixelShader (psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gbufferPS);
        psBlob->Release(); psBlob = nullptr;

        // Input layout for lit meshes (PhongVertex = POSITION + NORMAL).
        D3D11_INPUT_ELEMENT_DESC litElems[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                           D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        dev->CreateInputLayout(litElems, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &gbufferLayout);
        vsBlob->Release(); vsBlob = nullptr;

        // Textured PS (same VS blob is gone – recompile for validation, or reuse layout from above).
        CompileShader(GBUFFER_TEXTURED_SHADER_PATH, "PSMain", "ps_5_0", &psBlob);
        dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gbufferTexPS);
        psBlob->Release(); psBlob = nullptr;

        // Terrain geometry shaders (POSITION + COLOR(UV)).
        CompileShader(GBUFFER_TERRAIN_SHADER_PATH, "VSMain", "vs_5_0", &vsBlob);
        CompileShader(GBUFFER_TERRAIN_SHADER_PATH, "PSMain", "ps_5_0", &psBlob);
        dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &gbufferTerrainVS);
        dev->CreatePixelShader (psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gbufferTerrainPS);
        psBlob->Release(); psBlob = nullptr;

        D3D11_INPUT_ELEMENT_DESC terrainElems[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                           D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        dev->CreateInputLayout(terrainElems, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &gbufferTerrainLayout);
        vsBlob->Release(); vsBlob = nullptr;

        // ---- Lighting pass shaders ----
        CompileShader(LIGHTING_SHADER_PATH, "VSMain", "vs_5_0", &vsBlob);
        CompileShader(LIGHTING_SHADER_PATH, "PSMain", "ps_5_0", &psBlob);
        dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &quadVS);
        dev->CreatePixelShader (psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &lightingPS);
        vsBlob->Release(); vsBlob = nullptr;
        psBlob->Release(); psBlob = nullptr;

        // ---- Constant buffers ----
        auto makeCB = [&](UINT size, ID3D11Buffer** out) {
            D3D11_BUFFER_DESC desc = {};
            desc.Usage          = D3D11_USAGE_DYNAMIC;
            desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.ByteWidth      = size;
            dev->CreateBuffer(&desc, nullptr, out);
        };
        makeCB(sizeof(GBufferCBData),  &gbufferCB);
        makeCB(sizeof(LightingCBData), &lightingCB);

        // ---- Pipeline states ----
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable    = TRUE;
        blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&blendDesc, &additiveBlend);

        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable    = FALSE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dev->CreateDepthStencilState(&dsDesc, &noDepthState);

        D3D11_RASTERIZER_DESC rastDesc = {};
        rastDesc.FillMode = D3D11_FILL_SOLID;
        rastDesc.CullMode = D3D11_CULL_NONE;
        dev->CreateRasterizerState(&rastDesc, &cullNoneRS);
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::DestroyResources() {
        gbuffer.DestroyResources();
        auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
        rel(gbufferVS);   rel(gbufferPS);      rel(gbufferTexPS);
        rel(gbufferLayout);
        rel(gbufferTerrainVS); rel(gbufferTerrainPS); rel(gbufferTerrainLayout);
        rel(gbufferCB);
        rel(quadVS);      rel(lightingPS);     rel(lightingCB);
        rel(additiveBlend); rel(noDepthState); rel(cullNoneRS);
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::UpdateLightingCB(const LightingCBData& data) {
        auto* ctx = game->renderer.deviceContext;
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        ctx->Map(lightingCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &data, sizeof(LightingCBData));
        ctx->Unmap(lightingCB, 0);
        ctx->VSSetConstantBuffers(0, 1, &lightingCB);
        ctx->PSSetConstantBuffers(0, 1, &lightingCB);
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::GeometryPass() {
        auto* ctx = game->renderer.deviceContext;

        gbuffer.Bind(ctx, game->renderer.depthStencilView);
        gbuffer.Clear(ctx);

        D3D11_VIEWPORT vp = {};
        vp.Width    = static_cast<float>(game->renderer.ScreenWidth);
        vp.Height   = static_cast<float>(game->renderer.ScreenHeight);
        vp.MaxDepth = 1.f;
        ctx->RSSetViewports(1, &vp);
        ctx->RSSetState(cullNoneRS);

        // ---- Non-textured lit meshes ----
        ctx->VSSetShader(gbufferVS, nullptr, 0);
        ctx->PSSetShader(gbufferPS, nullptr, 0);
        ctx->IASetInputLayout(gbufferLayout);
        for (auto* comp : game->Components) {
            auto* lit = dynamic_cast<LitMeshComponent*>(comp);
            if (!lit || lit->IsTextured()) continue;
            lit->DrawDeferred(ctx, gbufferCB);
        }

        // ---- Textured lit meshes (Katamari ball) — reuse same VS/layout ----
        ctx->PSSetShader(gbufferTexPS, nullptr, 0);
        for (auto* comp : game->Components) {
            auto* lit = dynamic_cast<LitMeshComponent*>(comp);
            if (!lit || !lit->IsTextured()) continue;
            lit->DrawDeferred(ctx, gbufferCB);
        }

        // ---- Terrain ----
        ctx->VSSetShader(gbufferTerrainVS, nullptr, 0);
        ctx->PSSetShader(gbufferTerrainPS, nullptr, 0);
        ctx->IASetInputLayout(gbufferTerrainLayout);
        for (auto* comp : game->Components) {
            auto* terrain = dynamic_cast<TerrainComponent*>(comp);
            if (!terrain) continue;
            terrain->DrawDeferred(ctx, gbufferCB);
        }

        // Unbind GBuffer RTVs before lighting pass uses them as SRVs.
        ID3D11RenderTargetView* nullRTVs[2] = {nullptr, nullptr};
        ctx->OMSetRenderTargets(2, nullRTVs, nullptr);
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::LightingPass() {
        auto* ctx = game->renderer.deviceContext;

        // Bind GBuffer SRVs at t0-t1, depth at t2.
        gbuffer.BindSRVs(ctx, 0);
        ctx->PSSetShaderResources(2, 1, &game->renderer.depthSRV);

        // Bind shadow sampler at s0 if available.
        if (shadowManager) {
            ID3D11SamplerState* samp = shadowManager->GetShadowSampler();
            ctx->PSSetSamplers(0, 1, &samp);
        }

        // Backbuffer RTV, no DSV (we read depth as SRV at t2, not as depth target).
        ctx->OMSetRenderTargets(1, &game->renderer.renderTargetView, nullptr);
        ctx->OMSetDepthStencilState(noDepthState, 0);
        ctx->OMSetBlendState(additiveBlend, nullptr, 0xffffffff);

        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        ctx->VSSetShader(quadVS, nullptr, 0);
        ctx->PSSetShader(lightingPS, nullptr, 0);
        ctx->RSSetState(cullNoneRS);

        // Fill frame-constant parts of LightingCBData.
        LightingCBData cb = {};
        const auto camData = game->GetCameraData();
        cb.invViewProj = (camData.viewMatrix * camData.projMatrix).Invert().Transpose();
        cb.view        = camData.viewMatrix.Transpose();
        const auto cp  = game->GetCamera()->GetPosition();
        cb.cameraPos   = {cp.x, cp.y, cp.z, 0.f};
        cb.screenSize  = {static_cast<float>(game->renderer.ScreenWidth),
                          static_cast<float>(game->renderer.ScreenHeight), 0.f, 0.f};
        cb.ambient     = {0.15f, 0.15f, 0.15f, 0.f};

        // Ambient pass first (adds ambient * diffuse onto cleared-black buffer).
        cb.lightType = LightAmbient;
        UpdateLightingCB(cb);
        ctx->Draw(4, 0);

        // Per-light additive passes.
        for (auto* light : game->GetLights()) {
            if (!light->active) continue;

            LightData ld = light->GetLightData();
            cb.lightDirOrPos = ld.dirOrPos;
            cb.lightColor    = ld.color;
            cb.lightType     = ld.type;
            cb.attenConst    = ld.attenConst;
            cb.attenLinear   = ld.attenLinear;
            cb.attenQuad     = ld.attenQuad;
            cb.shadowsEnabled = 0;

            if (ld.type == LightSpot) {
                auto* spot = static_cast<SpotLightComponent*>(light);
                auto dir = spot->direction;
                dir.Normalize();
                cb.spotDirection = {dir.x, dir.y, dir.z, spot->InnerCos()};
                cb.spotOuterCos  = spot->OuterCos();
            }

            if (ld.type == LightDirectional && shadowManager) {
                const auto* vps    = shadowManager->GetLightVPs();
                const auto* splits = shadowManager->GetCascadeSplits();
                cb.lightViewProj0  = vps[0];
                cb.lightViewProj1  = vps[1];
                cb.lightViewProj2  = vps[2];
                cb.cascadeSplits   = {splits[0], splits[1], splits[2], 0.f};
                cb.shadowsEnabled  = 1;
                shadowManager->BindShadowSRVsDeferred(ctx, 3);  // t3-t5
            }

            UpdateLightingCB(cb);
            ctx->Draw(4, 0);

            // Unbind shadow SRVs after directional light draw.
            if (ld.type == LightDirectional && shadowManager) {
                ID3D11ShaderResourceView* nulls[3] = {};
                ctx->PSSetShaderResources(3, 3, nulls);
            }
        }

        // Clean up pipeline state.
        ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        ctx->OMSetDepthStencilState(nullptr, 0);

        // Unbind GBuffer SRVs and depth SRV so the depth texture can be re-bound as DSV.
        gbuffer.UnbindSRVs(ctx, 0);
        ID3D11ShaderResourceView* nullDepth = nullptr;
        ctx->PSSetShaderResources(2, 1, &nullDepth);
    }
}

#include "shadow_map_component.hpp"
#include "lights/directional_light_component.hpp"
#include "lit_model_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include <d3dcompiler.h>
#include <iostream>
#include <algorithm>
#include <cfloat>
#include <cmath>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    ShadowMapComponent::ShadowMapComponent(Game* game, DirectionalLightComponent* light)
        : GameComponent(game), light(light)
    {}

    void ShadowMapComponent::Initialize() {
        auto* device = game->renderer.device.Get();

        // Cascade split planes (view-space Z) using a log/uniform blend
        ComputeCascadeSplits();

        // Shadow map textures (R32_TYPELESS can be bound as D32_FLOAT DSV and R32_FLOAT SRV)
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width            = SHADOW_MAP_SIZE;
        texDesc.Height           = SHADOW_MAP_SIZE;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = 1;
        texDesc.Format           = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format             = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        for (int i = 0; i < NUM_CASCADES; ++i) {
            device->CreateTexture2D(&texDesc, nullptr, &shadowTextures[i]);
            device->CreateDepthStencilView(shadowTextures[i], &dsvDesc, &shadowDSVs[i]);
            device->CreateShaderResourceView(shadowTextures[i], &srvDesc, &shadowSRVs[i]);
        }

        // Shadow depth-pass vertex shader
        ID3DBlob* vsBlob  = nullptr;
        ID3DBlob* errBlob = nullptr;
        HRESULT hr = D3DCompileFromFile(SHADOW_DEPTH_SHADER_PATH, nullptr, nullptr,
            "VSMain", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vsBlob, &errBlob);
        if (FAILED(hr)) {
            if (errBlob) { std::cerr << (char*)errBlob->GetBufferPointer(); errBlob->Release(); }
            else throw std::runtime_error("ShadowDepth.hlsl not found");
        }
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                   nullptr, &shadowVS);

        D3D11_INPUT_ELEMENT_DESC posElem = {
            "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        };
        device->CreateInputLayout(&posElem, 1,
            vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &shadowLayout);
        vsBlob->Release();

        // Per-object depth-pass CB (one 64-byte matrix)
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage          = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.ByteWidth      = sizeof(Matrix);
        device->CreateBuffer(&cbDesc, nullptr, &depthPassCB);

        // Shadow-params CB sent to PhongShader (b1)
        cbDesc.ByteWidth = sizeof(ShadowCBData);
        device->CreateBuffer(&cbDesc, nullptr, &shadowParamsCB);

        // Rasterizer state with depth bias to reduce shadow acne
        D3D11_RASTERIZER_DESC rastDesc = {};
        rastDesc.FillMode              = D3D11_FILL_SOLID;
        rastDesc.CullMode              = D3D11_CULL_NONE;
        rastDesc.DepthBias             = 10000;
        rastDesc.SlopeScaledDepthBias  = 2.0f;
        rastDesc.DepthClipEnable       = TRUE;
        device->CreateRasterizerState(&rastDesc, &shadowRastState);

        // Depth-stencil state
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable    = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
        device->CreateDepthStencilState(&dsDesc, &shadowDSState);

        // Comparison sampler: border=1 (lit) so out-of-bounds fragments are unshadowed
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.BorderColor[0] = sampDesc.BorderColor[1] =
        sampDesc.BorderColor[2] = sampDesc.BorderColor[3] = 1.0f;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
        device->CreateSamplerState(&sampDesc, &shadowSampler);
    }

    void ShadowMapComponent::ComputeCascadeSplits() {
        constexpr float camNear  = 0.001f;
        constexpr float camFar   = SHADOW_FAR;
        constexpr float lambda   = 0.75f;

        for (int i = 1; i <= NUM_CASCADES; ++i) {
            float f = static_cast<float>(i) / NUM_CASCADES;
            float logSplit     = camNear * powf(camFar / camNear, f);
            float uniformSplit = camNear + (camFar - camNear) * f;
            cascadeSplits[i - 1] = lambda * logSplit + (1.f - lambda) * uniformSplit;
        }
    }

    void ShadowMapComponent::ComputeLightMatrices() {
        auto camData = game->GetCameraData();
        Matrix invVP = (camData.viewMatrix * camData.projMatrix).Invert();

        // Full view-frustum corners in NDC (DirectX: z in [0,1])
        static const DirectX::XMFLOAT3 ndc[8] = {
            {-1,-1, 0}, {1,-1, 0}, {-1, 1, 0}, {1, 1, 0},
            {-1,-1, 1}, {1,-1, 1}, {-1, 1, 1}, {1, 1, 1},
        };

        Vector3 worldCorners[8];
        for (int i = 0; i < 8; ++i) {
            Vector4 v{ndc[i].x, ndc[i].y, ndc[i].z, 1.f};
            v = Vector4::Transform(v, invVP);
            v /= v.w;
            worldCorners[i] = {v.x, v.y, v.z};
        }

        // Build light view matrix (directional: position along toward-light direction)
        Vector3 lightDir{light->direction.x, light->direction.y, light->direction.z};
        lightDir.Normalize();

        Vector3 lightPos = lightDir * 100.f;  // eye of the light
        Vector3 up = (fabsf(lightDir.y) > 0.99f) ? Vector3{1,0,0} : Vector3{0,1,0};
        Matrix lightView = Matrix::CreateLookAt(lightPos, Vector3::Zero, up);

        float prevSplit = 0.001f;
        for (int c = 0; c < NUM_CASCADES; ++c) {
            float nearFrac = prevSplit      / SHADOW_FAR;
            float farFrac  = cascadeSplits[c] / SHADOW_FAR;

            // Sub-frustum corners by linear interpolation along the frustum rays
            Vector3 sub[8];
            for (int i = 0; i < 4; ++i) {
                Vector3 ray = worldCorners[i + 4] - worldCorners[i];
                sub[i]     = worldCorners[i] + ray * nearFrac;
                sub[i + 4] = worldCorners[i] + ray * farFrac;
            }

            // AABB in light space
            float minX =  FLT_MAX, maxX = -FLT_MAX;
            float minY =  FLT_MAX, maxY = -FLT_MAX;
            float minZ =  FLT_MAX, maxZ = -FLT_MAX;

            for (auto& sc : sub) {
                Vector4 lc = Vector4::Transform(Vector4{sc.x, sc.y, sc.z, 1.f}, lightView);
                minX = min(minX, lc.x); maxX = max(maxX, lc.x);
                minY = min(minY, lc.y); maxY = max(maxY, lc.y);
                minZ = min(minZ, lc.z); maxZ = max(maxZ, lc.z);
            }

            // Extend Z backward to catch shadow casters behind the view frustum
            constexpr float zSlack = 20.f;
            Matrix lightProj = Matrix::CreateOrthographicOffCenter(
                minX, maxX, minY, maxY, minZ - zSlack, maxZ);

            lightVP[c] = lightView * lightProj;
            prevSplit = cascadeSplits[c];
        }
    }

    void ShadowMapComponent::RenderShadowMaps() {
        ComputeLightMatrices();

        auto* ctx = game->renderer.deviceContext;

        D3D11_VIEWPORT vp = {};
        vp.Width    = static_cast<float>(SHADOW_MAP_SIZE);
        vp.Height   = static_cast<float>(SHADOW_MAP_SIZE);
        vp.MaxDepth = 1.f;
        ctx->RSSetViewports(1, &vp);
        ctx->RSSetState(shadowRastState);
        ctx->OMSetDepthStencilState(shadowDSState, 0);
        ctx->VSSetShader(shadowVS, nullptr, 0);
        ctx->PSSetShader(nullptr, nullptr, 0);
        ctx->IASetInputLayout(shadowLayout);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetConstantBuffers(0, 1, &depthPassCB);

        for (int c = 0; c < NUM_CASCADES; ++c) {
            ctx->OMSetRenderTargets(0, nullptr, shadowDSVs[c]);
            ctx->ClearDepthStencilView(shadowDSVs[c], D3D11_CLEAR_DEPTH, 1.f, 0);

            for (auto* comp : game->Components) {
                if (auto* lit = dynamic_cast<LitModelComponent*>(comp)) {
                    Matrix wlvp = (lit->GetWorldMatrix() * lightVP[c]);
                    D3D11_MAPPED_SUBRESOURCE mapped;
                    ctx->Map(depthPassCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                    memcpy(mapped.pData, &wlvp, sizeof(Matrix));
                    ctx->Unmap(depthPassCB, 0);
                    lit->DrawDepth();
                }
            }
        }

        // Update the per-frame shadow params CB consumed by PhongShader
        ShadowCBData sd{};
        sd.lightViewProj0   = lightVP[0];
        sd.lightViewProj1   = lightVP[1];
        sd.lightViewProj2   = lightVP[2];
        sd.cascadeSplits    = {cascadeSplits[0], cascadeSplits[1], cascadeSplits[2], 0.f};
        sd.shadowsEnabled   = 1;

        D3D11_MAPPED_SUBRESOURCE mapped;
        ctx->Map(shadowParamsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &sd, sizeof(ShadowCBData));
        ctx->Unmap(shadowParamsCB, 0);

        ctx->OMSetDepthStencilState(nullptr, 0);
    }

    void ShadowMapComponent::BindForDraw(ID3D11DeviceContext* ctx) const {
        ctx->PSSetConstantBuffers(1, 1, &shadowParamsCB);
        ID3D11ShaderResourceView* srvs[3] = {shadowSRVs[0], shadowSRVs[1], shadowSRVs[2]};
        ctx->PSSetShaderResources(0, 3, srvs);
        ctx->PSSetSamplers(0, 1, &shadowSampler);
    }

    void ShadowMapComponent::DestroyResources() {
        for (int i = 0; i < NUM_CASCADES; ++i) {
            if (shadowSRVs[i])     { shadowSRVs[i]->Release();     shadowSRVs[i]     = nullptr; }
            if (shadowDSVs[i])     { shadowDSVs[i]->Release();     shadowDSVs[i]     = nullptr; }
            if (shadowTextures[i]) { shadowTextures[i]->Release(); shadowTextures[i] = nullptr; }
        }
        if (shadowVS)        { shadowVS->Release();        shadowVS        = nullptr; }
        if (shadowLayout)    { shadowLayout->Release();    shadowLayout    = nullptr; }
        if (depthPassCB)     { depthPassCB->Release();     depthPassCB     = nullptr; }
        if (shadowParamsCB)  { shadowParamsCB->Release();  shadowParamsCB  = nullptr; }
        if (shadowRastState) { shadowRastState->Release(); shadowRastState = nullptr; }
        if (shadowDSState)   { shadowDSState->Release();   shadowDSState   = nullptr; }
        if (shadowSampler)   { shadowSampler->Release();   shadowSampler   = nullptr; }
    }
}

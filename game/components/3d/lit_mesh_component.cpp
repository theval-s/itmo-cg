#include "lit_mesh_component.hpp"
#include "shadow_map_component.hpp"
#include "lights/light_component.hpp"
#include "game.hpp"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    LitMeshComponent::LitMeshComponent(Game* game) : MeshComponent(game) {}

    void LitMeshComponent::InitLitBuffers() {
        auto* dev = game->renderer.device.Get();
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage          = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.ByteWidth      = sizeof(PhongCBData);
        dev->CreateBuffer(&cbDesc, nullptr, &phongCB);
        cbDesc.ByteWidth = sizeof(ShadowCBData);
        dev->CreateBuffer(&cbDesc, nullptr, &shadowParamsCB);
    }

    void LitMeshComponent::BindPhongCB(const Matrix& worldMat) {
        const auto camData = game->GetCameraData();
        cbData.worldViewProj = (worldMat * camData.viewMatrix * camData.projMatrix).Transpose();
        cbData.world         = worldMat.Transpose();
        const Vector3 cp     = game->GetCamera()->GetPosition();
        cbData.cameraPos     = {cp.x, cp.y, cp.z, 0.f};

        const auto& lights = game->GetLights();
        int lcount = 0;
        for (size_t i = 0; i < lights.size() && lcount < MAX_LIGHTS; ++i)
            if (lights[i]->active)
                cbData.lights[lcount++] = lights[i]->GetLightData();
        cbData.lightCount = lcount;

        auto* ctx = game->renderer.deviceContext;
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        ctx->Map(phongCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &cbData, sizeof(PhongCBData));
        ctx->Unmap(phongCB, 0);
        ctx->VSSetConstantBuffers(0, 1, &phongCB);
        ctx->PSSetConstantBuffers(0, 1, &phongCB);
    }

    void LitMeshComponent::BindShadow() {
        auto* shadowMgr = game->GetShadowManager();
        if (shadowMgr) {
            shadowMgr->BindForDraw(game->renderer.deviceContext);
        } else {
            shadowCBData.shadowsEnabled = 0;
            auto* ctx = game->renderer.deviceContext;
            D3D11_MAPPED_SUBRESOURCE smap = {};
            ctx->Map(shadowParamsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &smap);
            memcpy(smap.pData, &shadowCBData, sizeof(ShadowCBData));
            ctx->Unmap(shadowParamsCB, 0);
            ctx->PSSetConstantBuffers(1, 1, &shadowParamsCB);
        }
    }

    void LitMeshComponent::DrawDepth() {
        auto* ctx = game->renderer.deviceContext;
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        constexpr UINT stride = sizeof(PhongVertex);
        constexpr UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        ctx->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    }
}
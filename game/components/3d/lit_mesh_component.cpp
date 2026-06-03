#include "lit_mesh_component.hpp"
#include "shadow_map_component.hpp"
#include "lights/light_component.hpp"
#include "game.hpp"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    LitMeshComponent::LitMeshComponent(Game* game) : MeshComponent(game) {}

    void LitMeshComponent::InitLitBuffers() {
        auto* dev = game->GetDevice();
        phongCB        = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(PhongCBData),  /*dynamic*/true});
        shadowParamsCB = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(ShadowCBData), /*dynamic*/true});
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

        phongCB->Update(&cbData, sizeof(PhongCBData));
        auto* cmd = game->GetCommandList();
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, phongCB);
        cmd->SetConstantBuffer(rhi::ShaderStage::Pixel,  0, phongCB);
    }

    void LitMeshComponent::BindShadow() {
        auto* shadowMgr = game->GetShadowManager();
        if (shadowMgr) {
            shadowMgr->BindForDraw(game->GetCommandList());
        } else {
            shadowCBData.shadowsEnabled = 0;
            shadowParamsCB->Update(&shadowCBData, sizeof(ShadowCBData));
            game->GetCommandList()->SetConstantBuffer(rhi::ShaderStage::Pixel, 1, shadowParamsCB);
        }
    }

    void LitMeshComponent::DrawDeferred(rhi::CommandList* cmd, rhi::GpuBuffer* gbCB) {
        const auto camData = game->GetCameraData();
        GBufferCBData cb = {};
        cb.worldViewProj = (worldMatrix * camData.viewMatrix * camData.projMatrix).Transpose();
        cb.world         = worldMatrix.Transpose();
        cb.matDiffuse    = cbData.matDiffuse;
        cb.matSpecular   = cbData.matSpecular;

        gbCB->Update(&cb, sizeof(GBufferCBData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, gbCB);
        cmd->SetConstantBuffer(rhi::ShaderStage::Pixel,  0, gbCB);

        cmd->SetVertexBuffer(vb, sizeof(PhongVertex));
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);
        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }

    void LitMeshComponent::DrawDepth(rhi::CommandList* cmd) {
        cmd->SetVertexBuffer(vb, sizeof(PhongVertex));
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);
        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }
}

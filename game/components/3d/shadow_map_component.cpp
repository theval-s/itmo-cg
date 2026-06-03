#include "shadow_map_component.hpp"
#include "lights/directional_light_component.hpp"
#include "lit_mesh_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include <algorithm>
#include <cfloat>
#include <cmath>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    ShadowMapComponent::ShadowMapComponent(Game* game, DirectionalLightComponent* light)
        : GameComponent(game), light(light)
    {}

    void ShadowMapComponent::Initialize() {
        auto* dev = game->GetDevice();

        ComputeCascadeSplits();

        // Per-cascade sampleable depth targets.
        rhi::TextureDesc dt;
        dt.width  = SHADOW_MAP_SIZE;
        dt.height = SHADOW_MAP_SIZE;
        dt.usage  = rhi::TextureUsage::DepthStencil | rhi::TextureUsage::ShaderResource;
        for (int i = 0; i < NUM_CASCADES; ++i)
            shadowDepth[i] = dev->CreateDepthTarget(dt, &shadowTex[i]);

        // VS-only depth pipeline. Depth bias fights acne; clamp instead of clip.
        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(SHADOW_DEPTH_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = nullptr;
        pd.layout.attributes = { {"POSITION", 0, rhi::VertexFormat::Float4} };
        pd.raster.cull                = rhi::CullMode::None;
        pd.raster.depthClip           = false;
        pd.raster.depthBias           = 5000;
        pd.raster.slopeScaledDepthBias = 1.0f;
        pd.depth.depthTest  = true;
        pd.depth.depthWrite = true;
        pd.depth.func       = rhi::CompareFunc::LessEqual;
        shadowPipeline = dev->CreatePipeline(pd);

        depthPassCB    = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(Matrix),       /*dynamic*/true});
        shadowParamsCB = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(ShadowCBData), /*dynamic*/true});

        // Comparison sampler, border = 1 (lit) so out-of-bounds fragments are unshadowed.
        shadowSampler = dev->CreateSampler({rhi::Filter::Comparison, rhi::AddressMode::Border,
                                            rhi::CompareFunc::LessEqual, /*borderColor*/1.f});

        // Debug overlay pipeline (SV_VertexID quad, no depth test).
        rhi::PipelineDesc dbg;
        dbg.vs = dev->CreateShader(DEBUG_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        dbg.ps = dev->CreateShader(DEBUG_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
        dbg.raster.cull     = rhi::CullMode::None;
        dbg.depth.depthTest  = false;
        dbg.depth.depthWrite = false;
        debugPipeline = dev->CreatePipeline(dbg);
    }

    void ShadowMapComponent::ComputeCascadeSplits() {
        constexpr float camNear  = 0.1f;
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

        static const DirectX::XMFLOAT3 ndc[8] = {
            {-1,-1, 0}, {1,-1, 0}, {-1, 1, 0}, {1, 1, 0},
            {-1,-1, 1}, {1,-1, 1}, {-1, 1, 1}, {1, 1, 1},
        };

        Vector3 worldCorners[8];
        for (int i = 0; i < 8; ++i) {
            Vector4 v{ndc[i].x, ndc[i].y, ndc[i].z, 1.f};
            v = Vector4::Transform(v, invVP);
            v /= v.w;
            worldCorners[i] = Vector3(v.x, v.y, v.z);
        }

        Vector3 lightDir{light->direction.x, light->direction.y, light->direction.z};
        lightDir.Normalize();

        Vector3 lightPos = lightDir * 100.f;
        Vector3 up = (fabsf(lightDir.y) > 0.99f) ? Vector3{1,0,0} : Vector3{0,1,0};
        Matrix lightView = Matrix::CreateLookAt(lightPos, Vector3::Zero, up);

        constexpr float CAM_FAR = SHADOW_FAR;
        float prevSplit = 0.001f;
        for (int c = 0; c < NUM_CASCADES; ++c) {
            float nearFrac = prevSplit        / CAM_FAR;
            float farFrac  = cascadeSplits[c] / CAM_FAR;

            Vector3 sub[8];
            for (int i = 0; i < 4; ++i) {
                Vector3 ray = worldCorners[i + 4] - worldCorners[i];
                sub[i]     = worldCorners[i] + ray * nearFrac;
                sub[i + 4] = worldCorners[i] + ray * farFrac;
            }

            float minX =  FLT_MAX, maxX = -FLT_MAX;
            float minY =  FLT_MAX, maxY = -FLT_MAX;
            float minZ =  FLT_MAX, maxZ = -FLT_MAX;

            for (auto& sc : sub) {
                Vector4 lc = Vector4::Transform(Vector4{sc.x, sc.y, sc.z, 1.f}, lightView);
                minX = min(minX, lc.x); maxX = max(maxX, lc.x);
                minY = min(minY, lc.y); maxY = max(maxY, lc.y);
                minZ = min(minZ, lc.z); maxZ = max(maxZ, lc.z);
            }

            constexpr float zSlack = 20.f;
            Matrix lightProj = Matrix::CreateOrthographicOffCenter(
                minX, maxX, minY, maxY, -maxZ - zSlack, -minZ + zSlack);

            lightVP[c] = lightView * lightProj;
            prevSplit = cascadeSplits[c];
        }
    }

    void ShadowMapComponent::RenderShadowMaps() {
        ComputeLightMatrices();
        auto* cmd = game->GetCommandList();

        // Unbind shadow SRVs (forward t0-t2, deferred t3-t5) before writing depth.
        cmd->UnbindTextures(rhi::ShaderStage::Pixel, 0, 6);

        cmd->SetViewport(0.f, 0.f, static_cast<float>(SHADOW_MAP_SIZE), static_cast<float>(SHADOW_MAP_SIZE));
        cmd->SetPipeline(shadowPipeline);

        for (int c = 0; c < NUM_CASCADES; ++c) {
            cmd->SetRenderTargets(nullptr, 0, shadowDepth[c]);
            cmd->ClearDepth(shadowDepth[c], 1.f);

            for (auto* comp : game->Components) {
                if (auto* lit = dynamic_cast<LitMeshComponent*>(comp)) {
                    Matrix wlvp = (lit->GetWorldMatrix() * lightVP[c]).Transpose();
                    depthPassCB->Update(&wlvp, sizeof(Matrix));
                    cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, depthPassCB);
                    lit->DrawDepth(cmd);
                }
            }
        }

        // Per-frame shadow params CB consumed by PhongShader (forward path).
        ShadowCBData sd{};
        sd.lightViewProj0 = lightVP[0].Transpose();
        sd.lightViewProj1 = lightVP[1].Transpose();
        sd.lightViewProj2 = lightVP[2].Transpose();
        sd.cascadeSplits  = {cascadeSplits[0], cascadeSplits[1], cascadeSplits[2], 0.f};
        sd.shadowsEnabled = 1;
        shadowParamsCB->Update(&sd, sizeof(ShadowCBData));
    }

    void ShadowMapComponent::DrawDebugShadowMaps() {
        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(debugPipeline);

        for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
            cmd->SetViewport(static_cast<float>(game->GetWidth() - 256),
                             static_cast<float>(cascade * (256 + 10)),
                             256.f, 256.f);
            cmd->SetTexture(rhi::ShaderStage::Pixel, 0, shadowTex[cascade]);
            cmd->Draw(6);
        }
    }

    void ShadowMapComponent::BindForDraw(rhi::CommandList* cmd) const {
        cmd->SetConstantBuffer(rhi::ShaderStage::Pixel, 1, shadowParamsCB);
        for (int i = 0; i < NUM_CASCADES; ++i)
            cmd->SetTexture(rhi::ShaderStage::Pixel, i, shadowTex[i]);
        cmd->SetSampler(rhi::ShaderStage::Pixel, 0, shadowSampler);
    }

    void ShadowMapComponent::BindShadowSRVsDeferred(rhi::CommandList* cmd, int startSlot) const {
        for (int i = 0; i < NUM_CASCADES; ++i)
            cmd->SetTexture(rhi::ShaderStage::Pixel, startSlot + i, shadowTex[i]);
    }
}

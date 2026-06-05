#include "rendering_system.hpp"
#include "game.hpp"
#include "consts.hpp"
#include "components/3d/lit_mesh_component.hpp"
#include "components/3d/katamari_player_component.hpp"
#include "components/3d/shadow_map_component.hpp"
#include "components/3d/lights/spot_light_component.hpp"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    RenderingSystem::RenderingSystem(Game* g) : game(g) {}

    // -------------------------------------------------------------------------
    void RenderingSystem::Initialize() {
        auto* dev = game->GetDevice();
        gbuffer.Initialize(dev, game->GetWidth(), game->GetHeight());

        const std::vector<rhi::VertexAttribute> litLayout = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"NORMAL",   0, rhi::VertexFormat::Float4},
        };
        const std::vector<rhi::VertexAttribute> terrainLayout = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"COLOR",    0, rhi::VertexFormat::Float4},
        };

        // Non-textured lit meshes.
        {
            rhi::PipelineDesc pd;
            pd.vs = dev->CreateShader(GBUFFER_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
            pd.ps = dev->CreateShader(GBUFFER_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
            pd.layout.attributes = litLayout;
            pd.raster.cull = rhi::CullMode::None;
            gbufferPipe = dev->CreatePipeline(pd);
        }
        // Textured lit meshes — own VS (emits local objPos for spherical UV) + textured PS.
        {
            rhi::PipelineDesc pd;
            pd.vs = dev->CreateShader(GBUFFER_TEXTURED_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
            pd.ps = dev->CreateShader(GBUFFER_TEXTURED_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
            pd.layout.attributes = litLayout;
            pd.raster.cull = rhi::CullMode::None;
            gbufferTexPipe = dev->CreatePipeline(pd);
        }
        // Terrain.
        {
            rhi::PipelineDesc pd;
            pd.vs = dev->CreateShader(GBUFFER_TERRAIN_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
            pd.ps = dev->CreateShader(GBUFFER_TERRAIN_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
            pd.layout.attributes = terrainLayout;
            pd.raster.cull = rhi::CullMode::None;
            terrainPipe = dev->CreatePipeline(pd);
        }
        // Lighting: fullscreen quad, additive blend, no depth.
        {
            rhi::PipelineDesc pd;
            pd.vs = dev->CreateShader(LIGHTING_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
            pd.ps = dev->CreateShader(LIGHTING_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
            pd.raster.cull = rhi::CullMode::None;
            pd.blend.enable   = true;
            pd.blend.srcColor = rhi::BlendFactor::One;
            pd.blend.dstColor = rhi::BlendFactor::One;
            pd.blend.opColor  = rhi::BlendOp::Add;
            pd.blend.srcAlpha = rhi::BlendFactor::One;
            pd.blend.dstAlpha = rhi::BlendFactor::Zero;
            pd.blend.opAlpha  = rhi::BlendOp::Add;
            pd.depth.depthTest  = false;
            pd.depth.depthWrite = false;
            pd.topology = rhi::PrimitiveTopology::TriangleStrip;
            lightingPipe = dev->CreatePipeline(pd);
        }

        gbufferCB  = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(GBufferCBData),  /*dynamic*/true});
        lightingCB = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(LightingCBData), /*dynamic*/true});
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::UpdateLightingCB(const LightingCBData& data) {
        auto* cmd = game->GetCommandList();
        lightingCB->Update(&data, sizeof(LightingCBData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, lightingCB);
        cmd->SetConstantBuffer(rhi::ShaderStage::Pixel,  0, lightingCB);
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::GeometryPass() {
        auto* dev = game->GetDevice();
        auto* cmd = game->GetCommandList();

        gbuffer.BindAsTargets(cmd, dev->GetDepthBuffer());
        gbuffer.Clear(cmd);
        cmd->SetViewport(0.f, 0.f, static_cast<float>(game->GetWidth()), static_cast<float>(game->GetHeight()));

        // Non-textured lit meshes.
        cmd->SetPipeline(gbufferPipe);
        for (auto* comp : game->Components) {
            auto* lit = dynamic_cast<LitMeshComponent*>(comp);
            if (!lit || lit->IsTextured()) continue;
            lit->DrawDeferred(cmd, gbufferCB);
        }

        // Textured lit meshes (Katamari ball).
        cmd->SetPipeline(gbufferTexPipe);
        for (auto* comp : game->Components) {
            auto* lit = dynamic_cast<LitMeshComponent*>(comp);
            if (!lit || !lit->IsTextured()) continue;
            lit->DrawDeferred(cmd, gbufferCB);
        }

        // Terrain.
        cmd->SetPipeline(terrainPipe);
        for (auto* comp : game->Components) {
            auto* terrain = dynamic_cast<TerrainComponent*>(comp);
            if (!terrain) continue;
            terrain->DrawDeferred(cmd, gbufferCB);
        }

        // Unbind targets before the G-buffer textures are read in the lighting pass.
        cmd->SetRenderTargets(nullptr, 0, nullptr);
    }

    // -------------------------------------------------------------------------
    void RenderingSystem::LightingPass() {
        auto* dev = game->GetDevice();
        auto* cmd = game->GetCommandList();

        // G-buffer SRVs at t0-t1, depth at t2.
        cmd->SetTexture(rhi::ShaderStage::Pixel, 0, gbuffer.DiffuseSpec());
        cmd->SetTexture(rhi::ShaderStage::Pixel, 1, gbuffer.Normal());
        cmd->SetTexture(rhi::ShaderStage::Pixel, 2, dev->GetDepthTexture());
        if (shadowManager)
            cmd->SetSampler(rhi::ShaderStage::Pixel, 0, shadowManager->GetShadowSampler());

        rhi::GpuRenderTarget* bb = dev->GetBackbuffer();
        cmd->SetRenderTargets(&bb, 1, nullptr);  // read depth as SRV, not as target
        cmd->SetViewport(0.f, 0.f, static_cast<float>(game->GetWidth()), static_cast<float>(game->GetHeight()));
        cmd->SetPipeline(lightingPipe);

        // Frame-constant parts.
        LightingCBData cb = {};
        const auto camData = game->GetCameraData();
        cb.invViewProj = (camData.viewMatrix * camData.projMatrix).Invert().Transpose();
        cb.view        = camData.viewMatrix.Transpose();
        const auto cp  = game->GetCamera()->GetPosition();
        cb.cameraPos   = {cp.x, cp.y, cp.z, 0.f};
        cb.screenSize  = {static_cast<float>(game->GetWidth()),
                          static_cast<float>(game->GetHeight()), 0.f, 0.f};
        cb.ambient     = {0.15f, 0.15f, 0.15f, 0.f};

        // Ambient pass.
        cb.lightType = LightAmbient;
        UpdateLightingCB(cb);
        cmd->Draw(4);

        // Per-light additive passes.
        for (auto* light : game->GetLights()) {
            if (!light->active) continue;

            LightData ld = light->GetLightData();
            cb.lightDirOrPos  = ld.dirOrPos;
            cb.lightColor     = ld.color;
            cb.lightType      = ld.type;
            cb.attenConst     = ld.attenConst;
            cb.attenLinear    = ld.attenLinear;
            cb.attenQuad      = ld.attenQuad;
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
                cb.lightViewProj0 = vps[0].Transpose();
                cb.lightViewProj1 = vps[1].Transpose();
                cb.lightViewProj2 = vps[2].Transpose();
                cb.cascadeSplits  = {splits[0], splits[1], splits[2], 0.f};
                cb.shadowsEnabled = 1;
                shadowManager->BindShadowSRVsDeferred(cmd, 3);  // t3-t5
            }

            UpdateLightingCB(cb);
            cmd->Draw(4);

            if (ld.type == LightDirectional && shadowManager)
                cmd->UnbindTextures(rhi::ShaderStage::Pixel, 3, 3);
        }

        // Unbind G-buffer + depth SRVs so the depth texture can be re-bound as DSV.
        cmd->UnbindTextures(rhi::ShaderStage::Pixel, 0, 3);
    }
}

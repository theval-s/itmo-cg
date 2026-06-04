//
// Created by Val on 04.06.2026.
//
#include "particle_system_component.hpp"

#include <vector>

#include "game.hpp"
#include "consts.hpp"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    namespace {
        constexpr unsigned THREADS = 64;
        unsigned DivUp(unsigned n, unsigned d) { return (n + d - 1) / d; }
    }

    ParticleSystemComponent::ParticleSystemComponent(Game* game,
                                                     Vector3 origin,
                                                     float emissionRate,
                                                     int maxParticles)
        : GameComponent(game),
          origin(origin),
          emissionRate(emissionRate),
          maxParticles(static_cast<uint32_t>(maxParticles)) {}

    // -------------------------------------------------------------------------
    void ParticleSystemComponent::Initialize() {
        auto* dev = game->GetDevice();

        // Compute shaders: emit + simulate (share the draw shader file).
        csEmit     = dev->CreateShader(PARTICLE_SHADER_PATH, "CSEmit",     rhi::ShaderStage::Compute);
        csSimulate = dev->CreateShader(PARTICLE_SHADER_PATH, "CSSimulate", rhi::ShaderStage::Compute);

        // Draw pipeline: no vertex buffer (VS expands quads via SV_VertexID),
        // additive blend, depth-tested but not depth-writing.
        {
            rhi::PipelineDesc pd;
            pd.vs = dev->CreateShader(PARTICLE_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
            pd.ps = dev->CreateShader(PARTICLE_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
            pd.raster.cull    = rhi::CullMode::None;
            pd.blend.enable   = true;
            pd.blend.srcColor = rhi::BlendFactor::SrcAlpha;
            pd.blend.dstColor = rhi::BlendFactor::One;
            pd.blend.opColor  = rhi::BlendOp::Add;
            pd.blend.srcAlpha = rhi::BlendFactor::One;
            pd.blend.dstAlpha = rhi::BlendFactor::One;
            pd.blend.opAlpha  = rhi::BlendOp::Add;
            pd.depth.depthTest  = true;
            pd.depth.depthWrite = false;
            drawPipe = dev->CreatePipeline(pd);
        }

        // Particle pool — zero-initialised so every slot starts dead (energy == 0).
        std::vector<GpuParticle> zeros(maxParticles);
        rhi::BufferDesc pbd;
        pbd.type            = rhi::BufferType::Structured;
        pbd.byteWidth       = sizeof(GpuParticle) * maxParticles;
        pbd.structureStride = sizeof(GpuParticle);
        pbd.uav             = true;
        particleBuffer = dev->CreateBuffer(pbd, zeros.data());

        // Alive list — append buffer of indices into the pool.
        rhi::BufferDesc abd;
        abd.type            = rhi::BufferType::Structured;
        abd.byteWidth       = sizeof(uint32_t) * maxParticles;
        abd.structureStride = sizeof(uint32_t);
        abd.uav             = true;
        abd.append          = true;
        aliveList = dev->CreateBuffer(abd, nullptr);

        // Indirect draw args: {IndexCountPerInstance=6, InstanceCount=0, start..=0}.
        // The simulate pass overwrites InstanceCount with the GPU-computed count.
        const uint32_t args[5] = {6, 0, 0, 0, 0};
        indirectArgs = dev->CreateBuffer({rhi::BufferType::IndirectArgs, sizeof(args)}, args);

        // One quad template (two triangles). The fetched index value is the corner id.
        const uint32_t quad[6] = {0, 1, 2, 0, 2, 3};
        indexBuffer = dev->CreateBuffer({rhi::BufferType::Index, sizeof(quad)}, quad);

        cb = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(ParticleCBData), /*dynamic*/true});
    }

    // -------------------------------------------------------------------------
    void ParticleSystemComponent::Update(float deltaTime) {
        lastDt       = deltaTime;
        runningTime += deltaTime;

        // Accumulate fractional particles, emit whole ones; advance the ring cursor.
        accumulatedTime += emissionRate * deltaTime;
        uint32_t emit = static_cast<uint32_t>(accumulatedTime);
        accumulatedTime -= static_cast<float>(emit);
        if (emit > maxParticles) emit = maxParticles;

        pendingEmit = emit;
        emitHead    = (emitHead + emit) % maxParticles;
    }

    // -------------------------------------------------------------------------
    void ParticleSystemComponent::Draw() {
        auto* cmd = game->GetCommandList();

        // ---- Fill the shared constant buffer ----
        const auto camData = game->GetCameraData();
        Vector3 fwd = game->GetCamera()->GetForward();
        Vector3 right = Vector3::Up.Cross(fwd);
        if (right.LengthSquared() < 1e-6f) right = Vector3::Right;
        right.Normalize();
        Vector3 up = fwd.Cross(right);
        up.Normalize();

        ParticleCBData data{};
        data.viewProj     = (camData.viewMatrix * camData.projMatrix).Transpose();
        data.camRight     = {right.x, right.y, right.z, 0.f};
        data.camUp        = {up.x, up.y, up.z, 0.f};
        data.origin       = {origin.x, origin.y, origin.z, 0.f};
        data.force        = {force.x, force.y, force.z, 0.f};
        data.dt           = lastDt;
        data.emitCount    = pendingEmit;
        // The ring cursor was already advanced in Update(); rewind to the spawn base.
        data.emitHead     = (emitHead + maxParticles - pendingEmit) % maxParticles;
        data.maxParticles = maxParticles;
        data.time         = runningTime;
        cb->Update(&data, sizeof(ParticleCBData));

        // ---- Emit pass ----
        cmd->SetConstantBuffer(rhi::ShaderStage::Compute, 0, cb);
        if (pendingEmit > 0) {
            cmd->SetComputeShader(csEmit);
            cmd->SetComputeUAV(0, particleBuffer);
            cmd->Dispatch(DivUp(pendingEmit, THREADS));
        }

        // ---- Simulate pass (resets the alive-list counter to 0) ----
        cmd->SetComputeShader(csSimulate);
        cmd->SetComputeUAV(0, particleBuffer);
        cmd->SetComputeUAV(1, aliveList, /*initialCount*/0);
        cmd->Dispatch(DivUp(maxParticles, THREADS));

        // GPU-only count -> indirect InstanceCount (offset 4 bytes).
        cmd->CopyStructureCount(indirectArgs, 4, aliveList);

        // Release UAVs so the buffers can be read as SRVs by the draw.
        cmd->UnbindComputeUAVs(0, 2);

        // ---- Indirect draw ----
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, cb);
        cmd->SetBufferSRV(rhi::ShaderStage::Vertex, 0, particleBuffer);
        cmd->SetBufferSRV(rhi::ShaderStage::Vertex, 1, aliveList);
        cmd->SetPipeline(drawPipe);
        cmd->SetIndexBuffer(indexBuffer, rhi::IndexFormat::Uint32);
        cmd->DrawIndexedInstancedIndirect(indirectArgs, 0);

        cmd->UnbindTextures(rhi::ShaderStage::Vertex, 0, 2);
    }
}

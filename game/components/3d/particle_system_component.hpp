//
// GPU-driven billboard particle system — a single fountain emitter.
//
// Mirrors the lecture-slide structure:
//   * Particle  — pos, prevPos, velocity, acceleration, energy, size, sizeDelta,
//                 weight, weightDelta, color, colorDelta. Lives entirely on the GPU.
//   * the system owns the particle list, maxParticles, the (GPU-only) live count,
//     an origin, an emission rate / accumulated time, and a force (gravity/wind).
//
// Per frame: an emit compute pass spawns particles into a ring buffer, a simulate
// compute pass integrates them and appends survivors to an alive list, the alive
// count is copied into an indirect-args buffer, and the particles are drawn with
// DrawIndexedInstancedIndirect — each expanded to a camera-facing quad in the VS
// (no geometry shader). All GPU access goes through the RHI; no backend types here.
//
#pragma once
#include <cstdint>
#include "game_component.hpp"
#include "rhi/graphics_device.hpp"
#include "SimpleMath.h"

namespace val_cg {

    class ParticleSystemComponent : public GameComponent {
    public:
        ParticleSystemComponent(Game* game,
                                DirectX::SimpleMath::Vector3 origin,
                                float emissionRate = 600.f,
                                int   maxParticles = 8192);

        void Initialize() override;
        void Update(float deltaTime) override;
        void Draw() override;
        void DestroyResources() override {}   // GPU resources owned by the device

        // Render the alive particles into one shadow cascade as light-facing depth
        // billboards. Called by ShadowMapComponent during the shadow pass.
        void DrawShadowDepth(rhi::CommandList* cmd,
                             const DirectX::SimpleMath::Matrix& lightVP,
                             const DirectX::SimpleMath::Vector3& lightDir);

    private:
        // GPU particle record — byte-identical to the HLSL `Particle` struct.
        // Each float3 is paired with a scalar to fill a 16-byte row (avoids
        // structured-buffer packing surprises). 112 bytes.
        struct GpuParticle {
            DirectX::XMFLOAT3 pos;          float energy;
            DirectX::XMFLOAT3 prevPos;      float size;
            DirectX::XMFLOAT3 velocity;     float sizeDelta;
            DirectX::XMFLOAT3 acceleration; float weight;
            DirectX::XMFLOAT4 color;
            DirectX::XMFLOAT4 colorDelta;
            float             weightDelta;  DirectX::XMFLOAT3 _pad;
        };

        // Constant buffer shared by the compute + draw stages (matches HLSL ParticleCB).
        struct ParticleCBData {
            DirectX::SimpleMath::Matrix viewProj;
            DirectX::XMFLOAT4 camRight;
            DirectX::XMFLOAT4 camUp;
            DirectX::XMFLOAT4 origin;
            DirectX::XMFLOAT4 force;
            float    dt;
            uint32_t emitCount;
            uint32_t emitHead;
            uint32_t maxParticles;
            float    time;
            DirectX::XMFLOAT3 _cbpad;
        };

        // ---- System state (slide's ParticleSystem fields) ----
        DirectX::SimpleMath::Vector3 origin;
        DirectX::SimpleMath::Vector3 force{0.f, -9.8f, 0.f};   // gravity
        float    emissionRate;
        float    accumulatedTime = 0.f;   // fractional particles owed
        float    runningTime     = 0.f;   // RNG seed source
        uint32_t maxParticles;
        uint32_t emitHead   = 0;          // ring-buffer write cursor
        uint32_t pendingEmit = 0;         // particles to spawn this frame
        float    lastDt     = 0.f;

        // Shadow-pass constant buffer (matches HLSL ParticleShadowCB).
        struct ParticleShadowCBData {
            DirectX::SimpleMath::Matrix lightViewProj;
            DirectX::XMFLOAT4 lightDir;
        };

        // ---- GPU resources ----
        rhi::GpuShader* csEmit     = nullptr;
        rhi::GpuShader* csSimulate = nullptr;
        rhi::GpuPipeline* drawPipe = nullptr;
        rhi::GpuPipeline* shadowPipe = nullptr;  // depth-only, light-facing billboards

        rhi::GpuBuffer* particleBuffer = nullptr;  // RWStructuredBuffer<Particle> (the "particle list")
        rhi::GpuBuffer* aliveList      = nullptr;  // AppendStructuredBuffer<uint>
        rhi::GpuBuffer* indirectArgs   = nullptr;  // DrawIndexedInstancedIndirect args
        rhi::GpuBuffer* indexBuffer    = nullptr;  // 6 indices: one quad template
        rhi::GpuBuffer* cb             = nullptr;  // ParticleCBData
        rhi::GpuBuffer* shadowCB       = nullptr;  // ParticleShadowCBData (per cascade)
    };

}

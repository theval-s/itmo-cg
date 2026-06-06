#pragma once
#include "rhi/graphics_device.hpp"
#include "gbuffer.hpp"
#include "common_structs.h"

namespace val_cg {
    class Game;
    class ShadowMapComponent;

    // Deferred renderer: geometry pass fills the G-buffer, lighting pass shades
    // one fullscreen quad per light. Talks only to the RHI (no raw D3D11).
    class RenderingSystem {
    public:
        explicit RenderingSystem(Game* game);
        void Initialize();
        void DestroyResources() {}   // pipelines/buffers owned by the GraphicsDevice

        void GeometryPass();
        void LightingPass();

        // GPU picking: dispatch a compute shader that samples the G-buffer at the
        // clicked pixel, then read the id/world-pos/normal back and log it.
        // Must run after GeometryPass (the G-buffer must be filled this frame).
        void Pick(int clickX, int clickY);

        void SetShadowManager(ShadowMapComponent* sm) { shadowManager = sm; }

    private:
        void UpdateLightingCB(const LightingCBData& data);

        Game*               game          = nullptr;
        ShadowMapComponent* shadowManager = nullptr;
        GBuffer             gbuffer;

        // Geometry-pass pipelines (vs+ps+layout+state).
        rhi::GpuPipeline* gbufferPipe    = nullptr;  // POSITION+NORMAL, material colour
        rhi::GpuPipeline* gbufferTexPipe = nullptr;  // POSITION+NORMAL, textured (ball)
        rhi::GpuPipeline* terrainPipe    = nullptr;  // POSITION+COLOR(UV), terrain

        // Lighting-pass pipeline (fullscreen quad, additive, no depth).
        rhi::GpuPipeline* lightingPipe   = nullptr;

        rhi::GpuBuffer*   gbufferCB      = nullptr;  // GBufferCBData
        rhi::GpuBuffer*   lightingCB     = nullptr;  // LightingCBData

        // Picking resources.
        rhi::GpuShader*   pickCS         = nullptr;  // PickShader.hlsl CSMain
        rhi::GpuBuffer*   pickCB         = nullptr;  // PickCBData
        rhi::GpuBuffer*   pickResultBuf  = nullptr;  // RWStructuredBuffer<PickResult> (u0)
        rhi::GpuBuffer*   pickStaging    = nullptr;  // CPU-readable copy of the result
    };
}

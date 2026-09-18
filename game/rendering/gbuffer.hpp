//
// G-buffer: the deferred geometry-pass render targets.
// (Not a GameComponent — it's a render-target bundle, hence game/rendering/.)
//
#pragma once
#include "rhi/graphics_device.hpp"

namespace val_cg {
    class GBuffer {
    public:
        // Target formats, in RT order. Single source of truth for both the texture
        // creation below and the geometry-pass PipelineDesc (Metal needs them at PSO time).
        static constexpr int NUM_TARGETS = 3;
        static constexpr rhi::TextureFormat FORMATS[NUM_TARGETS] = {
            rhi::TextureFormat::RGBA8_UNORM,    // RT0: diffuse rgb + packed shininess
            rhi::TextureFormat::RGBA16_FLOAT,   // RT1: world-space normal
            rhi::TextureFormat::R32_FLOAT,      // RT2: per-object id (0 = background)
        };
        static void SetPipelineFormats(rhi::PipelineDesc& pd) {
            for (int i = 0; i < NUM_TARGETS; ++i) pd.colorFormats[i] = FORMATS[i];
            pd.numColorTargets = NUM_TARGETS;
        }

        void Initialize(rhi::GraphicsDevice* device, int width, int height);
        void Clear(rhi::CommandList* cmd);

        // Bind RT0..RT2 as colour targets (plus the scene depth) for the geometry pass.
        void BindAsTargets(rhi::CommandList* cmd, rhi::GpuDepthTarget* depth);

        rhi::GpuTexture* DiffuseSpec() const { return tex0; }  // t0
        rhi::GpuTexture* Normal()      const { return tex1; }  // t1
        rhi::GpuTexture* ObjectId()    const { return tex2; }  // object-id (R32F, picking)

    private:
        rhi::GpuRenderTarget* rt0 = nullptr;  rhi::GpuTexture* tex0 = nullptr;  // DiffuseSpec
        rhi::GpuRenderTarget* rt1 = nullptr;  rhi::GpuTexture* tex1 = nullptr;  // Normal
        rhi::GpuRenderTarget* rt2 = nullptr;  rhi::GpuTexture* tex2 = nullptr;  // ObjectId
    };
}

//
// G-buffer: the deferred geometry-pass render targets.
// (Not a GameComponent — it's a render-target bundle, hence game/rendering/.)
//
#pragma once
#include "rhi/graphics_device.hpp"

namespace val_cg {
    class GBuffer {
    public:
        void Initialize(rhi::GraphicsDevice* device, int width, int height);
        void Clear(rhi::CommandList* cmd);

        // Bind RT0 + RT1 as colour targets (plus the scene depth) for the geometry pass.
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

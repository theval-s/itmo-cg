//
// G-buffer implementation.
//
#include "gbuffer.hpp"

namespace val_cg {

    void GBuffer::Initialize(rhi::GraphicsDevice* device, int width, int height) {
        rhi::TextureDesc d0;
        d0.width = width; d0.height = height;
        d0.format = rhi::TextureFormat::RGBA8_UNORM;   // diffuse rgb + packed shininess
        rt0 = device->CreateRenderTarget(d0, &tex0);

        rhi::TextureDesc d1;
        d1.width = width; d1.height = height;
        d1.format = rhi::TextureFormat::RGBA16_FLOAT;  // world-space normal
        rt1 = device->CreateRenderTarget(d1, &tex1);

        rhi::TextureDesc d2;
        d2.width = width; d2.height = height;
        d2.format = rhi::TextureFormat::R32_FLOAT;      // per-object id (0 = background)
        rt2 = device->CreateRenderTarget(d2, &tex2);
    }

    void GBuffer::Clear(rhi::CommandList* cmd) {
        const float black[4] = {0.f, 0.f, 0.f, 0.f};
        cmd->ClearRenderTarget(rt0, black);
        cmd->ClearRenderTarget(rt1, black);
        cmd->ClearRenderTarget(rt2, black);   // id 0 = nothing picked
    }

    void GBuffer::BindAsTargets(rhi::CommandList* cmd, rhi::GpuDepthTarget* depth) {
        rhi::GpuRenderTarget* rts[3] = {rt0, rt1, rt2};
        cmd->SetRenderTargets(rts, 3, depth);
    }
}

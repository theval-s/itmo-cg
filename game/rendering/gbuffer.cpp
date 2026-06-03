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
    }

    void GBuffer::Clear(rhi::CommandList* cmd) {
        const float black[4] = {0.f, 0.f, 0.f, 0.f};
        cmd->ClearRenderTarget(rt0, black);
        cmd->ClearRenderTarget(rt1, black);
    }

    void GBuffer::BindAsTargets(rhi::CommandList* cmd, rhi::GpuDepthTarget* depth) {
        rhi::GpuRenderTarget* rts[2] = {rt0, rt1};
        cmd->SetRenderTargets(rts, 2, depth);
    }
}

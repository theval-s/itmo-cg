//
// Render Hardware Interface — command recording.
//
// Today this wraps the D3D11 immediate context; on Metal/D3D12 it maps onto a
// command encoder / command list. Components and the render systems talk to
// this instead of touching a device context directly.
//
#pragma once
#include "rhi_enums.hpp"
#include "rhi_resources.hpp"

namespace val_cg::rhi {

    class CommandList {
    public:
        virtual ~CommandList() = default;

        // ---- Targets / clears ----
        // Bind up to `count` colour targets plus an optional depth target.
        virtual void SetRenderTargets(GpuRenderTarget* const* targets, int count,
                                      GpuDepthTarget* depth) = 0;
        virtual void ClearRenderTarget(GpuRenderTarget* target, const float rgba[4]) = 0;
        virtual void ClearDepth(GpuDepthTarget* depth, float value) = 0;
        virtual void SetViewport(float x, float y, float width, float height) = 0;

        // ---- Pipeline + bindings ----
        virtual void SetPipeline(GpuPipeline* pipeline) = 0;
        virtual void SetVertexBuffer(GpuBuffer* vb, unsigned stride) = 0;
        virtual void SetIndexBuffer(GpuBuffer* ib, IndexFormat format) = 0;
        virtual void SetConstantBuffer(ShaderStage stage, unsigned slot, GpuBuffer* cb) = 0;
        virtual void SetTexture(ShaderStage stage, unsigned slot, GpuTexture* tex) = 0;
        virtual void SetSampler(ShaderStage stage, unsigned slot, GpuSampler* samp) = 0;

        // ---- Draws ----
        virtual void Draw(unsigned vertexCount, unsigned startVertex = 0) = 0;
        virtual void DrawIndexed(unsigned indexCount, unsigned startIndex = 0,
                                 int baseVertex = 0) = 0;

        // Convenience: unbind a contiguous range of textures (avoids RTV/SRV hazards).
        virtual void UnbindTextures(ShaderStage stage, unsigned slot, int count) = 0;
    };

}

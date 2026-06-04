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

        // Bind a structured/raw buffer as a shader-resource view (read-only) — the
        // GPU-side counterpart to SetTexture for buffers (e.g. particle pool in the VS).
        virtual void SetBufferSRV(ShaderStage stage, unsigned slot, GpuBuffer* buf) = 0;

        // ---- Draws ----
        virtual void Draw(unsigned vertexCount, unsigned startVertex = 0) = 0;
        virtual void DrawIndexed(unsigned indexCount, unsigned startIndex = 0,
                                 int baseVertex = 0) = 0;
        // Instanced indexed draw whose args (index/instance counts) are read from a
        // GPU buffer — the count never round-trips to the CPU.
        virtual void DrawIndexedInstancedIndirect(GpuBuffer* args, unsigned argsOffset = 0) = 0;

        // ---- Compute ----
        virtual void SetComputeShader(GpuShader* cs) = 0;
        // Bind a UAV to the compute stage. `initialCount` resets an append/consume
        // counter (0xffffffff = keep current).
        virtual void SetComputeUAV(unsigned slot, GpuBuffer* buf,
                                   unsigned initialCount = 0xffffffffu) = 0;
        virtual void Dispatch(unsigned groupsX, unsigned groupsY = 1, unsigned groupsZ = 1) = 0;
        // Copy the hidden append-counter of `appendBuffer` into `dst` at `dstOffset`
        // (used to feed an indirect-draw InstanceCount from a GPU-computed count).
        virtual void CopyStructureCount(GpuBuffer* dst, unsigned dstOffset,
                                        GpuBuffer* appendBuffer) = 0;

        // Convenience: unbind a contiguous range of textures (avoids RTV/SRV hazards).
        virtual void UnbindTextures(ShaderStage stage, unsigned slot, int count) = 0;
        // Unbind a contiguous range of compute UAVs (so the buffers can be read as SRVs).
        virtual void UnbindComputeUAVs(unsigned slot, int count) = 0;
    };

}

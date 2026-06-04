//
// D3D11 backend — CommandList over the immediate device context.
//
#pragma once
#include <d3d11.h>
#include "../command_list.hpp"

namespace val_cg::rhi::d3d11 {

    class D3D11CommandList : public CommandList {
    public:
        explicit D3D11CommandList(ID3D11DeviceContext* ctx) : ctx(ctx) {}

        void SetRenderTargets(GpuRenderTarget* const* targets, int count, GpuDepthTarget* depth) override;
        void ClearRenderTarget(GpuRenderTarget* target, const float rgba[4]) override;
        void ClearDepth(GpuDepthTarget* depth, float value) override;
        void SetViewport(float x, float y, float width, float height) override;

        void SetPipeline(GpuPipeline* pipeline) override;
        void SetVertexBuffer(GpuBuffer* vb, unsigned stride) override;
        void SetIndexBuffer(GpuBuffer* ib, IndexFormat format) override;
        void SetConstantBuffer(ShaderStage stage, unsigned slot, GpuBuffer* cb) override;
        void SetTexture(ShaderStage stage, unsigned slot, GpuTexture* tex) override;
        void SetSampler(ShaderStage stage, unsigned slot, GpuSampler* samp) override;
        void SetBufferSRV(ShaderStage stage, unsigned slot, GpuBuffer* buf) override;

        void Draw(unsigned vertexCount, unsigned startVertex) override;
        void DrawIndexed(unsigned indexCount, unsigned startIndex, int baseVertex) override;
        void DrawIndexedInstancedIndirect(GpuBuffer* args, unsigned argsOffset) override;

        void SetComputeShader(GpuShader* cs) override;
        void SetComputeUAV(unsigned slot, GpuBuffer* buf, unsigned initialCount) override;
        void Dispatch(unsigned groupsX, unsigned groupsY, unsigned groupsZ) override;
        void CopyStructureCount(GpuBuffer* dst, unsigned dstOffset, GpuBuffer* appendBuffer) override;

        void UnbindTextures(ShaderStage stage, unsigned slot, int count) override;
        void UnbindComputeUAVs(unsigned slot, int count) override;

    private:
        ID3D11DeviceContext* ctx = nullptr;  // non-owning
    };

}

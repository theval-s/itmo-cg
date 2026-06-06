//
// Render Hardware Interface — abstract GPU resource types.
//
// These are opaque handles from a component's point of view: the device owns
// them, components hold non-owning pointers. Each backend (D3D11 today, Metal
// later) subclasses them with its concrete native objects.
//
#pragma once
#include <cstddef>

namespace val_cg::rhi {

    // Compiled shader program for a single stage.
    class GpuShader {
    public:
        virtual ~GpuShader() = default;
    };

    // Vertex / index / constant buffer.
    class GpuBuffer {
    public:
        virtual ~GpuBuffer() = default;
        // Upload `bytes` from `data` (dynamic buffers only — map/discard).
        virtual void Update(const void* data, size_t bytes) = 0;
        // Copy `bytes` from a readback (staging) buffer into `dst`. Blocks until the
        // GPU has finished producing the data (map with CPU read). No-op otherwise.
        virtual void Readback(void* dst, size_t bytes) {}
    };

    // Immutable bundle of shaders + vertex layout + fixed-function state.
    class GpuPipeline {
    public:
        virtual ~GpuPipeline() = default;
    };

    // Sampled texture (also the read-side view of render targets / depth).
    class GpuTexture {
    public:
        virtual ~GpuTexture() = default;
    };

    // Writable colour attachment.
    class GpuRenderTarget {
    public:
        virtual ~GpuRenderTarget() = default;
    };

    // Writable depth/stencil attachment.
    class GpuDepthTarget {
    public:
        virtual ~GpuDepthTarget() = default;
    };

    class GpuSampler {
    public:
        virtual ~GpuSampler() = default;
    };

}

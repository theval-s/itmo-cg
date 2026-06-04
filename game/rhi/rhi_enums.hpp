//
// Render Hardware Interface — API-agnostic enums.
// NOTHING in game/rhi/*.hpp may include d3d11.h or any backend header.
//
#pragma once

namespace val_cg::rhi {

    enum class ShaderStage {
        Vertex,
        Pixel,
        Compute,
    };

    enum class CullMode {
        None,
        Front,
        Back,
    };

    enum class FillMode {
        Solid,
        Wireframe,
    };

    enum class CompareFunc {
        Never,
        Less,
        LessEqual,
        Equal,
        Greater,
        GreaterEqual,
        Always,
    };

    enum class BlendFactor {
        Zero,
        One,
        SrcAlpha,
        InvSrcAlpha,
    };

    enum class BlendOp {
        Add,
        Subtract,
    };

    enum class PrimitiveTopology {
        TriangleList,
        TriangleStrip,
        LineList,
        PointList,
    };

    // Vertex-attribute element formats for input layouts.
    enum class VertexFormat {
        Float2,
        Float3,
        Float4,
    };

    enum class IndexFormat {
        Uint16,
        Uint32,
    };

    enum class TextureFormat {
        RGBA8_UNORM,
        BGRA8_UNORM,      // swapchain backbuffer
        RGBA16_FLOAT,     // gbuffer normals
        R32_FLOAT,        // depth as SRV
        D32_FLOAT,        // depth-stencil view
        R32_TYPELESS,     // depth texture (viewable as both D32 and R32)
    };

    // Bit flags describing how a texture will be used (combine with operator|).
    enum class TextureUsage : unsigned {
        None           = 0,
        ShaderResource = 1 << 0,
        RenderTarget   = 1 << 1,
        DepthStencil   = 1 << 2,
    };

    inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
    }
    inline bool operator&(TextureUsage a, TextureUsage b) {
        return (static_cast<unsigned>(a) & static_cast<unsigned>(b)) != 0;
    }

    enum class BufferType {
        Vertex,
        Index,
        Constant,
        Structured,    // GPU-addressable array (RWStructuredBuffer / StructuredBuffer)
        IndirectArgs,  // source of args for indirect draws (DrawIndexedInstancedIndirect)
    };

    // Sampler filtering / addressing.
    enum class Filter {
        Point,
        Linear,
        Comparison,   // for shadow PCF (SampleCmp)
    };

    enum class AddressMode {
        Wrap,
        Clamp,
        Border,
    };

}

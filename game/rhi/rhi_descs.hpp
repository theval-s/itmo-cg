//
// Render Hardware Interface — API-agnostic descriptor structs.
//
#pragma once
#include "rhi_enums.hpp"
#include <vector>
#include <cstddef>
#include <cstdint>

namespace val_cg::rhi {

    class GpuShader;  // forward (referenced by PipelineDesc)

    // One vertex-buffer attribute. offset == APPEND packs it after the previous one.
    struct VertexAttribute {
        const char*  semantic      = "";
        unsigned     semanticIndex = 0;
        VertexFormat format        = VertexFormat::Float4;
        unsigned     offset        = APPEND;

        static constexpr unsigned APPEND = 0xffffffffu;
    };

    struct VertexLayoutDesc {
        std::vector<VertexAttribute> attributes;
    };

    struct RasterizerDesc {
        CullMode cull = CullMode::Back;
        FillMode fill = FillMode::Solid;
        bool     depthClip = true;       // false = clamp out-of-range depth (shadow pass)
        int      depthBias = 0;
        float    slopeScaledDepthBias = 0.f;

        bool operator==(const RasterizerDesc&) const = default;
    };

    struct BlendDesc {
        bool        enable   = false;
        BlendFactor srcColor = BlendFactor::One;
        BlendFactor dstColor = BlendFactor::Zero;
        BlendOp     opColor  = BlendOp::Add;
        BlendFactor srcAlpha = BlendFactor::One;
        BlendFactor dstAlpha = BlendFactor::Zero;
        BlendOp     opAlpha  = BlendOp::Add;

        bool operator==(const BlendDesc&) const = default;
    };

    struct DepthStencilDesc {
        bool        depthTest  = true;
        bool        depthWrite = true;
        CompareFunc func       = CompareFunc::Less;

        bool operator==(const DepthStencilDesc&) const = default;
    };

    struct SamplerDesc {
        Filter      filter     = Filter::Linear;
        AddressMode address    = AddressMode::Wrap;
        CompareFunc compare    = CompareFunc::LessEqual;  // used when filter == Comparison
        float       borderColor = 1.f;                    // used when address == Border (shadow PCF)

        bool operator==(const SamplerDesc&) const = default;
    };

    struct BufferDesc {
        BufferType  type    = BufferType::Vertex;
        size_t      byteWidth = 0;
        bool        dynamic = false;   // CPU-writable each frame (constant buffers)

        // ---- Structured-buffer extras (compute / GPU-driven particles) ----
        unsigned    structureStride = 0;  // element size for Structured buffers
        bool        uav     = false;      // create an unordered-access view (compute-writable)
        bool        append  = false;      // UAV carries a hidden append/consume counter
        bool        readback = false;     // CPU-readable staging buffer (CopyBuffer dst + Readback)
    };

    struct TextureDesc {
        int           width  = 0;
        int           height = 0;
        TextureFormat format = TextureFormat::RGBA8_UNORM;
        TextureUsage  usage  = TextureUsage::ShaderResource;
        int           arraySize = 1;
    };

    // A complete pipeline: shaders + vertex layout + fixed-function state.
    // Maps to a Metal/D3D12 PSO; on D3D11 the backend expands it into the
    // individual VS/PS/IA/RS/OM state objects (deduplicated by the state cache).
    struct PipelineDesc {
        GpuShader*        vs       = nullptr;
        GpuShader*        ps       = nullptr;
        VertexLayoutDesc  layout;
        RasterizerDesc    raster;
        BlendDesc         blend;
        DepthStencilDesc  depth;
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    };

}

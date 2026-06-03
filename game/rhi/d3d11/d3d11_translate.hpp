//
// D3D11 backend — inline translation from RHI enums to D3D11 enums.
// Only backend files include this; it is the one place that knows both vocabularies.
//
#pragma once
#include <d3d11.h>
#include "../rhi_enums.hpp"

namespace val_cg::rhi::d3d11 {

    inline DXGI_FORMAT ToDXGI(TextureFormat f) {
        switch (f) {
            case TextureFormat::RGBA8_UNORM:  return DXGI_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::BGRA8_UNORM:  return DXGI_FORMAT_B8G8R8A8_UNORM;
            case TextureFormat::RGBA16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case TextureFormat::R32_FLOAT:    return DXGI_FORMAT_R32_FLOAT;
            case TextureFormat::D32_FLOAT:    return DXGI_FORMAT_D32_FLOAT;
            case TextureFormat::R32_TYPELESS: return DXGI_FORMAT_R32_TYPELESS;
        }
        return DXGI_FORMAT_UNKNOWN;
    }

    inline DXGI_FORMAT ToDXGI(VertexFormat f) {
        switch (f) {
            case VertexFormat::Float2: return DXGI_FORMAT_R32G32_FLOAT;
            case VertexFormat::Float3: return DXGI_FORMAT_R32G32B32_FLOAT;
            case VertexFormat::Float4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        return DXGI_FORMAT_UNKNOWN;
    }

    inline DXGI_FORMAT ToDXGI(IndexFormat f) {
        return f == IndexFormat::Uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    }

    inline D3D11_CULL_MODE ToD3D(CullMode c) {
        switch (c) {
            case CullMode::None:  return D3D11_CULL_NONE;
            case CullMode::Front: return D3D11_CULL_FRONT;
            case CullMode::Back:  return D3D11_CULL_BACK;
        }
        return D3D11_CULL_NONE;
    }

    inline D3D11_FILL_MODE ToD3D(FillMode f) {
        return f == FillMode::Wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    }

    inline D3D11_COMPARISON_FUNC ToD3D(CompareFunc f) {
        switch (f) {
            case CompareFunc::Never:        return D3D11_COMPARISON_NEVER;
            case CompareFunc::Less:         return D3D11_COMPARISON_LESS;
            case CompareFunc::LessEqual:    return D3D11_COMPARISON_LESS_EQUAL;
            case CompareFunc::Equal:        return D3D11_COMPARISON_EQUAL;
            case CompareFunc::Greater:      return D3D11_COMPARISON_GREATER;
            case CompareFunc::GreaterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
            case CompareFunc::Always:       return D3D11_COMPARISON_ALWAYS;
        }
        return D3D11_COMPARISON_LESS;
    }

    inline D3D11_BLEND ToD3D(BlendFactor b) {
        switch (b) {
            case BlendFactor::Zero:        return D3D11_BLEND_ZERO;
            case BlendFactor::One:         return D3D11_BLEND_ONE;
            case BlendFactor::SrcAlpha:    return D3D11_BLEND_SRC_ALPHA;
            case BlendFactor::InvSrcAlpha: return D3D11_BLEND_INV_SRC_ALPHA;
        }
        return D3D11_BLEND_ONE;
    }

    inline D3D11_BLEND_OP ToD3D(BlendOp o) {
        return o == BlendOp::Subtract ? D3D11_BLEND_OP_SUBTRACT : D3D11_BLEND_OP_ADD;
    }

    inline D3D11_PRIMITIVE_TOPOLOGY ToD3D(PrimitiveTopology t) {
        switch (t) {
            case PrimitiveTopology::TriangleList:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case PrimitiveTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case PrimitiveTopology::LineList:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case PrimitiveTopology::PointList:     return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        }
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }

    inline D3D11_TEXTURE_ADDRESS_MODE ToD3D(AddressMode a) {
        switch (a) {
            case AddressMode::Wrap:   return D3D11_TEXTURE_ADDRESS_WRAP;
            case AddressMode::Clamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
            case AddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
        }
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }

}

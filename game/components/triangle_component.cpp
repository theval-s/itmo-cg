//
// Created by Volkov Sergey on 26/02/2026.
//

#include "triangle_component.hpp"
#include "../game.hpp"

#include <directxmath.h>

namespace val_cg {
    TriangleComponent::TriangleComponent(Game* game, std::span<const DirectX::XMFLOAT4> inputPoints, std::span<const int> inputIndices)
    : GameComponent(game) {
        if (inputPoints.empty() || inputIndices.empty()) {
            points = {
                //position, color
                DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
                DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
                DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
                DirectX::XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
            };
            indices = {0,1,2, 1,0,3};
        }
        else {
            points.assign_range(inputPoints);
            indices.assign_range(inputIndices);
        }
    }

    void TriangleComponent::Initialize() {
        auto* dev = game->GetDevice();

        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(VERTEX_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = dev->CreateShader(PIXEL_SHADER_PATH,  "PSMain", rhi::ShaderStage::Pixel);
        pd.layout.attributes = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"COLOR",    0, rhi::VertexFormat::Float4},
        };
        pd.raster.cull = rhi::CullMode::None;
        pipeline = dev->CreatePipeline(pd);

        vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(DirectX::XMFLOAT4) * points.size()}, points.data());
        ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},            indices.data());
    }

    void TriangleComponent::Draw() {
        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(pipeline);
        cmd->SetVertexBuffer(vb, 32);  // float4 position + float4 color
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);
        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }

    // Resources are owned by the GraphicsDevice and freed when it is destroyed.
    void TriangleComponent::DestroyResources() {}
} // val_cg

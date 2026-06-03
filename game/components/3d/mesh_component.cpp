//
// Created by Val on 30.03.2026.
//

#include "mesh_component.hpp"
#include "game.hpp"
#include "consts.hpp"

namespace val_cg {
    MeshComponent::MeshComponent(Game *game, std::span<const DirectX::XMFLOAT4> inputPoints,
        std::span<const int> inputIndices)
            : TriangleComponent(game, inputPoints, inputIndices) {
    }

    void MeshComponent::Initialize() {
        auto* dev = game->GetDevice();

        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(SPHERE_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = dev->CreateShader(SPHERE_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
        pd.layout.attributes = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"COLOR",    0, rhi::VertexFormat::Float4},
        };
        pd.raster.cull = rhi::CullMode::None;
        pd.topology    = topology;   // planets/orbits override this (line list)
        pipeline = dev->CreatePipeline(pd);

        vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(DirectX::XMFLOAT4) * points.size()}, points.data());
        ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},            indices.data());
        constantBuffer = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(WorldViewProjData), /*dynamic*/true});
    }

    void MeshComponent::Draw() {
        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(pipeline);
        cmd->SetVertexBuffer(vb, 32);  // float4 position + float4 color
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);

        constantBuffer->Update(&data, sizeof(WorldViewProjData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, constantBuffer);

        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }

    void MeshComponent::Update(float deltaTime) {
        const auto camData = game->GetCameraData();
        data.matrix = (worldMatrix * camData.viewMatrix * camData.projMatrix).Transpose();
    }
}//val_cg

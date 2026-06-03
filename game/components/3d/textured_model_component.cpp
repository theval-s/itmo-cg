//
// Created by Val on 25.05.2026.
//

#include "textured_model_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include <iostream>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    namespace { struct TexVert { DirectX::XMFLOAT4 pos; DirectX::XMFLOAT2 uv; }; }

    TexturedModelComponent::TexturedModelComponent(Game* game,
                                                   const std::string& modelPath,
                                                   const std::wstring& texturePath,
                                                   Vector3 position,
                                                   float scale)
        : MeshComponent(game)
        , ModelComponent(game, modelPath, position, scale, {1.f, 1.f, 1.f, 1.f})
        , texturePath(texturePath)
    {}

    void TexturedModelComponent::Initialize() {
        auto* dev = game->GetDevice();

        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(TEXTURED_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = dev->CreateShader(TEXTURED_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
        pd.layout.attributes = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"TEXCOORD", 0, rhi::VertexFormat::Float2},
        };
        pd.raster.cull = rhi::CullMode::None;
        pipeline = dev->CreatePipeline(pd);

        constantBuffer = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(WorldViewProjData), /*dynamic*/true});

        // Build interleaved {float4 pos, float2 uv}. points[] is {pos,color} pairs.
        vertexCount = static_cast<int>(points.size()) / 2;
        std::vector<TexVert> texVerts;
        texVerts.reserve(vertexCount);
        for (int i = 0; i < vertexCount; ++i) {
            TexVert tv;
            tv.pos = points[i * 2];
            tv.uv  = (i < static_cast<int>(meshUVs.size())) ? meshUVs[i]
                                                            : DirectX::XMFLOAT2{0.f, 0.f};
            texVerts.push_back(tv);
        }

        vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(TexVert) * texVerts.size()}, texVerts.data());
        ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},      indices.data());

        srv     = dev->CreateTextureFromFile(texturePath.c_str());
        sampler = dev->CreateSampler({rhi::Filter::Linear, rhi::AddressMode::Wrap});

        collider.Radius = baseBoundingRadius * scaleVal;
        collider.Center = {worldPos.x, worldPos.y, worldPos.z};
    }

    void TexturedModelComponent::Draw() {
        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(pipeline);
        cmd->SetVertexBuffer(vb, sizeof(TexVert));
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);

        constantBuffer->Update(&data, sizeof(WorldViewProjData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, constantBuffer);

        cmd->SetTexture(rhi::ShaderStage::Pixel, 0, srv);
        cmd->SetSampler(rhi::ShaderStage::Pixel, 0, sampler);

        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }
}

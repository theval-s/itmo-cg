//
// Created by Val on 22.03.2026.
//

#include "paddle_component.hpp"

#include <iostream>
#include <algorithm>
#include "directxmath.h"
#include "../game.hpp"
#include "consts.hpp"
#include "Keys.h"


constexpr DirectX::XMFLOAT4 paddle_polys[8] = {
    DirectX::XMFLOAT4(0.05f, 0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
    DirectX::XMFLOAT4(-0.05f, -0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
    DirectX::XMFLOAT4(0.05f, -0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
    DirectX::XMFLOAT4(-0.05f, 0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
};
constexpr int paddle_indices[6] = {0, 1, 2, 1, 0, 3};


val_cg::PaddleComponent::PaddleComponent(Game *game, Alignment al)
    : TriangleComponent(game, paddle_polys, paddle_indices)
      , alignment(al) {
    if (alignment == Left) {
        data.offset.x = -0.9;
    } else if (alignment == Right) {
        data.offset.x = 0.9;
    } else {
        alignment=Left;
        data.offset.x = -0.9;
    }
}

void val_cg::PaddleComponent::Initialize() {
    auto* dev = game->GetDevice();

    rhi::PipelineDesc pd;
    pd.vs = dev->CreateShader(PADDLE_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
    pd.ps = dev->CreateShader(PADDLE_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
    pd.layout.attributes = {
        {"POSITION", 0, rhi::VertexFormat::Float4},
        {"COLOR",    0, rhi::VertexFormat::Float4},
    };
    pd.raster.cull = rhi::CullMode::None;
    pipeline = dev->CreatePipeline(pd);

    constantBuffer = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(PaddleBuffer), /*dynamic*/true});
    vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(DirectX::XMFLOAT4) * points.size()}, points.data());
    ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},            indices.data());

    collider.Center.x=data.offset.x;
    collider.Center.y=0;
    collider.Extents.x=0.05f;
    collider.Extents.y=0.2f;
}

void val_cg::PaddleComponent::Draw() {
    auto* cmd = game->GetCommandList();
    cmd->SetPipeline(pipeline);
    cmd->SetVertexBuffer(vb, 32);
    cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);

    constantBuffer->Update(&data, sizeof(PaddleBuffer));
    cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, constantBuffer);

    cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
}

void val_cg::PaddleComponent::Update(float deltaTime) {
    if (alignment == Left) {
        if (GetAsyncKeyState(static_cast<int>(Keys::S))) {
            data.offset.y = std::clamp(data.offset.y - speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y - speed * deltaTime, -1.f, 1.f);
        } else if (GetAsyncKeyState(static_cast<int>(Keys::W))) {
            data.offset.y = std::clamp(data.offset.y + speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y + speed * deltaTime, -1.f, 1.f);
        }
    } else if (alignment == Right) {
        if (GetAsyncKeyState(static_cast<int>(Keys::Down))) {
            data.offset.y = std::clamp(data.offset.y - speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y - speed * deltaTime, -1.f, 1.f);
        } else if (GetAsyncKeyState(static_cast<int>(Keys::Up))) {
            data.offset.y = std::clamp(data.offset.y + speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y + speed * deltaTime, -1.f, 1.f);
        }
    } else {
        throw std::runtime_error("Alignment not supported");
    }
}

// Resources are owned by the GraphicsDevice.
void val_cg::PaddleComponent::DestroyResources() {}

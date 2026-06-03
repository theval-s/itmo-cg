//
// Created by Val on 22.03.2026.
//

#include "ball_component.hpp"

#include <iostream>
#include "game.hpp"
#include "directxmath.h"
#include "consts.hpp"
#include "paddle_component.hpp"

constexpr DirectX::XMFLOAT4 ball_polys[8] = {
    DirectX::XMFLOAT4(0.05f, 0.05f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
    DirectX::XMFLOAT4(-0.05f, -0.05f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
    DirectX::XMFLOAT4(0.05f, -0.05f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
    DirectX::XMFLOAT4(-0.05f, 0.05f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
};
constexpr int ball_indices[6] = {0, 1, 2, 1, 0, 3};

val_cg::BallComponent::BallComponent(Game *game)
: TriangleComponent(game, ball_polys, ball_indices)
{
}

void val_cg::BallComponent::Initialize() {
    auto* dev = game->GetDevice();

    rhi::PipelineDesc pd;
    pd.vs = dev->CreateShader(BALL_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
    pd.ps = dev->CreateShader(BALL_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
    pd.layout.attributes = {
        {"POSITION", 0, rhi::VertexFormat::Float4},
        {"COLOR",    0, rhi::VertexFormat::Float4},
    };
    pd.raster.cull = rhi::CullMode::None;
    pipeline = dev->CreatePipeline(pd);

    constantBuffer = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(BallBuffer), /*dynamic*/true});
    vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(DirectX::XMFLOAT4) * points.size()}, points.data());
    ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},            indices.data());

    collider.Center.x=0;
    collider.Center.y=0;
    collider.Extents.x = 0.05f;
    collider.Extents.y = 0.05f;

    ResetBall();
}

void val_cg::BallComponent::Draw() {
    auto* cmd = game->GetCommandList();
    cmd->SetPipeline(pipeline);
    cmd->SetVertexBuffer(vb, 32);
    cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);

    constantBuffer->Update(&data, sizeof(BallBuffer));
    cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, constantBuffer);

    cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
}

void val_cg::BallComponent::Update(float deltaTime) {

    if (data.offset.y <= -1.f || data.offset.y >= 1.f) v2 = -v2;

    if (data.offset.x <= -1.f || data.offset.x >= 1.f) {
        game->Scored();
        ResetBall();
    }
    for (const auto& component : game->Components) {
        if (auto paddle = dynamic_cast<PaddleComponent*>(component)) {
            if (collider.Intersects(paddle->collider)) {
                DirectX::SimpleMath::Vector2 v(collider.Center.x - paddle->collider.Center.x,
                                                collider.Center.y - paddle->collider.Center.y);
                v.Normalize();
                v1 = v.x;
                v2 = v.y;
                speed*=1.1f;
                break;
            }
        }
    }

    float deltaX = v1 * speed * deltaTime;
    float deltaY = v2 * speed * deltaTime;
    data.offset.x += deltaX;
    data.offset.y += deltaY;
    collider.Center.x += deltaX;
    collider.Center.y += deltaY;

}

void val_cg::BallComponent::ResetBall() {
    //get random move direction

    v1 = rand()%2 ? 1 : -1; //todo: more interesting?
    v2 = 0;
    speed = 1.f;

    data.offset.x = 0;
    data.offset.y = 0;
    collider.Center.x=0;
    collider.Center.y=0;
}

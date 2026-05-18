//
// Created by Val on 18.05.2026.
//

#include "ball_3d_component.hpp"
#include "game.hpp"
#include "paddle_3d_component.hpp"
#include "utils/geometry_generator.hpp"
#include "SimpleMath.h"
#include "paddle_3d_component.hpp"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    Ball3DComponent::Ball3DComponent(Game *game)
        : MeshComponent(game) {
        MeshData mesh = GeometryGenerator::CreateSphere(radius, 12, 12, {1.f, 1.f, 1.f, 1.f});
        points.reserve(mesh.vertices.size() * 2);
        for (const auto &v: mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color);
        }
        indices = mesh.indices;
        Reset();
        worldMatrix = Matrix::CreateTranslation(position);
    }

    void Ball3DComponent::Initialize() {
        MeshComponent::Initialize();
        collider.Center = {0.f, 0.f, 0.f};
        collider.Radius = radius;
    }

    void Ball3DComponent::Update(float deltaTime) {
        // Top/bottom wall bounce
        if (position.y + radius >= fieldHalfY || position.y - radius <= -fieldHalfY)
            velocity.y = -velocity.y;
        // Left/right scoring
        if (position.x < -fieldHalfX || position.x > fieldHalfX) {
            game->Scored();
            Reset();
        }
        // Paddle collision
        for (auto *comp: game->Components) {
            if (auto *paddle = dynamic_cast<Paddle3DComponent *>(comp)) {
                if (collider.Intersects(paddle->collider)) {
                    Vector3 paddleCenter(paddle->collider.Center.x,
                                         paddle->collider.Center.y,
                                         paddle->collider.Center.z);
                    Vector3 dir = position - paddleCenter;
                    dir.Normalize();
                    velocity = dir;
                    speed *= 1.1f;
                    break;
                }
            }
        }
        position += velocity * speed * deltaTime;
        collider.Center = {position.x, position.y, position.z};
        worldMatrix = Matrix::CreateTranslation(position);
        MeshComponent::Update(deltaTime);
    }

    void Ball3DComponent::Reset() {
        position = Vector3::Zero;
        velocity = Vector3(rand() % 2 ? 1.f : -1.f, 0.3f, 0.f);
        velocity.Normalize();
        speed = 3.0f;
        collider.Center = {0.f, 0.f, 0.f};
    }
}

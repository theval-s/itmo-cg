//
// Created by Val on 25.05.2026.
//

#include "katamari_player_component.hpp"
#include "model_component.hpp"
#include "game.hpp"
#include "utils/geometry_generator.hpp"
#include "Keys.h"
#include <cmath>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    KatamariFloorComponent::KatamariFloorComponent(Game* game) : MeshComponent(game) {
        //plane
        points.clear();
        indices.clear();
        MeshData mesh = GeometryGenerator::CreateBox(30.f, 0.2f, 30.f, {0.3f, 0.55f, 0.25f, 1.f});
        for (const auto& v : mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color);
        }
        indices = mesh.indices;
        worldMatrix = Matrix::CreateTranslation(0.f, -0.6f, 0.f);
    }

    void KatamariFloorComponent::Update(float deltaTime) {
        MeshComponent::Update(deltaTime);
    }

    KatamariPlayerComponent::KatamariPlayerComponent(Game* game)
        : MeshComponent(game)
    {
        //sphere
        points.clear();
        indices.clear();
        MeshData mesh = GeometryGenerator::CreateSphere(1.0f, 16, 16, {0.9f, 0.3f, 0.1f, 1.f});
        for (const auto& v : mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color);
        }
        indices = mesh.indices;
    }

    void KatamariPlayerComponent::Initialize() {
        MeshComponent::Initialize();
        rollMatrix = Matrix::Identity;
        position.y = groundY + radius;
        collider.Center = {position.x, position.y, position.z};
        collider.Radius = radius;
    }

    void KatamariPlayerComponent::Update(float deltaTime) {
        auto* input = game->InputHandler();
        Vector3 moveDir = Vector3::Zero;
        if (input) {
            if (input->IsKeyDown(Keys::Up))    moveDir.z += 1.f;
            if (input->IsKeyDown(Keys::Down))  moveDir.z -= 1.f;
            if (input->IsKeyDown(Keys::Left))  moveDir.x += 1.f;
            if (input->IsKeyDown(Keys::Right)) moveDir.x -= 1.f;
        }

        if (moveDir.LengthSquared() > 0.f) {
            moveDir.Normalize();
            float dist = speed * deltaTime;
            position += moveDir * dist;

            // Rolling without slipping: angle = arc_length / radius
            float rollAngle = dist / radius;
            Vector3 rollAxis(moveDir.z, 0.f, -moveDir.x);
            rollMatrix = Matrix::CreateFromAxisAngle(rollAxis, rollAngle) * rollMatrix;
        }

        CheckCollision();

        collider.Center = {position.x, position.y, position.z};
        collider.Radius = radius;

        worldMatrix = Matrix::CreateScale(radius) * rollMatrix * Matrix::CreateTranslation(position);
        MeshComponent::Update(deltaTime);
    }

    void KatamariPlayerComponent::CheckCollision() {
        for (auto* comp : game->Components) {
            auto* model = dynamic_cast<ModelComponent*>(comp);
            if (!model || model->IsAttached()) continue;

            if (collider.Intersects(model->GetCollider())) {
                if (model->GetObjectRadius() <= radius * 1.5f) {
                    const auto& c = model->GetCollider().Center;
                    Vector3 worldOffset = Vector3(c.x, c.y, c.z) - position;
                    model->AttachTo(&position, &rollMatrix, worldOffset);

                    // Volume-preserving growth: V_new = V_ball + V_model
                    float r3  = radius * radius * radius;
                    float mr3 = model->GetObjectRadius() * model->GetObjectRadius() * model->GetObjectRadius();
                    radius = std::cbrt(r3 + mr3);

                    //raise ball when it grows
                    position.y = groundY + radius;
                }
            }
        }
    }
}

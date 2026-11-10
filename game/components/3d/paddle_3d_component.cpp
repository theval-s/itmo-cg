//
// Created by Val on 18.05.2026.
//

#include "paddle_3d_component.hpp"
#include <algorithm>
#include "game.hpp"
#include "Keys.h"
#include "utils/geometry_generator.hpp"

namespace val_cg {
    using namespace val_cg::math;

    Paddle3DComponent::Paddle3DComponent(Game *game, Alignment al)
        : MeshComponent(game), alignment(al) {
        float x = (alignment == Left) ? -4.5f : 4.5f;
        position = Vector3(x, 0.f, 0.f);
        math::Float4 color = (alignment == Left)
                                      ? math::Float4(1.f, 0.2f, 0.2f, 1.f)
                                      : math::Float4(0.2f, 0.2f, 1.f, 1.f);
        MeshData mesh = GeometryGenerator::CreateBox(halfD, halfH, halfD, color);
        std::vector<math::Float4> tempPoints;
        tempPoints.reserve(mesh.vertices.size()*2);
        for (const auto& v : mesh.vertices) {
            tempPoints.push_back(v.position);
            tempPoints.push_back(v.color);
        }
        this->points = tempPoints;
        this->indices = mesh.indices;
        worldMatrix = Matrix::CreateTranslation(position);
    }

    void Paddle3DComponent::Initialize() {
        MeshComponent::Initialize();
        collider.Center = {position.x, position.y, position.z};
        collider.Extents = {halfD, halfH, halfD};
    }

    void Paddle3DComponent::Update(float deltaTime) {
        auto* input = game->InputHandler();
        float dy = 0.f;
        if (alignment == Left) {
            if (input->IsKeyDown(Keys::I)) dy = speed * deltaTime;
            if (input->IsKeyDown(Keys::K)) dy = -speed * deltaTime;
        } else {
            if (input->IsKeyDown(Keys::Up)) dy = speed * deltaTime;
            if (input->IsKeyDown(Keys::Down)) dy = -speed * deltaTime;
        }
        position.y = std::clamp(position.y + dy, -(fieldHalfY - halfH), fieldHalfY - halfH);
        collider.Center = {position.x, position.y, position.z};
        worldMatrix = Matrix::CreateTranslation(position);
        MeshComponent::Update(deltaTime);
    }
}

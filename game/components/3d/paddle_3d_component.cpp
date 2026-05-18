//
// Created by Val on 18.05.2026.
//

#include "paddle_3d_component.hpp"
#include <algorithm>
#include "game.hpp"
#include "Keys.h"
#include "utils/geometry_generator.hpp"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    Paddle3DComponent::Paddle3DComponent(Game *game, Alignment al)
        : MeshComponent(game), alignment(al) {
        float x = (alignment == Left) ? -4.5f : 4.5f;
        position = Vector3(x, 0.f, 0.f);
        DirectX::XMFLOAT4 color = (alignment == Left)
                                      ? DirectX::XMFLOAT4(1.f, 0.2f, 0.2f, 1.f)
                                      : DirectX::XMFLOAT4(0.2f, 0.2f, 1.f, 1.f);
        MeshData mesh = GeometryGenerator::CreateBox(halfD, halfH, halfD, color);
        std::vector<DirectX::XMFLOAT4> tempPoints;
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
        float dy = 0.f;
        if (alignment == Left) {
            if (GetAsyncKeyState(static_cast<int>(Keys::I))) dy = speed * deltaTime;
            if (GetAsyncKeyState(static_cast<int>(Keys::K))) dy = -speed * deltaTime;
        } else {
            if (GetAsyncKeyState(static_cast<int>(Keys::Up))) dy = speed * deltaTime;
            if (GetAsyncKeyState(static_cast<int>(Keys::Down))) dy = -speed * deltaTime;
        }
        position.y = std::clamp(position.y + dy, -(fieldHalfY - halfH), fieldHalfY - halfH);
        collider.Center = {position.x, position.y, position.z};
        worldMatrix = Matrix::CreateTranslation(position);
        MeshComponent::Update(deltaTime);
    }
}

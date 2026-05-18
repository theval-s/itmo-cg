//
// Created by Val on 13.05.2026.
//

#pragma once
#include "mesh_component.hpp"
#include "SimpleMath.h"
#include "DirectXCollision.h"

namespace val_cg {
    class Paddle3DComponent : public MeshComponent {
    public:
        enum Alignment { Left, Right };

        DirectX::BoundingBox collider;

        Paddle3DComponent(Game *game, Alignment alignment);

        void Initialize() override;

        void Update(float deltaTime) override;

    private:
        Alignment alignment;
        DirectX::SimpleMath::Vector3 position;

        static constexpr float speed = 3.0f;
        static constexpr float halfH = 1.0f;
        static constexpr float halfD = 0.15f;
        static constexpr float fieldHalfY = 3.0f;
    };
}


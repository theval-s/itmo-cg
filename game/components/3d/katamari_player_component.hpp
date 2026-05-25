//
// Created by Val on 25.05.2026.
//

#pragma once
#include "mesh_component.hpp"
#include <DirectXCollision.h>

namespace val_cg {
    // flat ground plane
    class KatamariFloorComponent : public MeshComponent {
    public:
        explicit KatamariFloorComponent(Game* game);
        void Update(float deltaTime) override;
    };

    // ball that acts as a player
    class KatamariPlayerComponent : public MeshComponent {
    public:
        explicit KatamariPlayerComponent(Game* game);

        void Initialize() override;
        void Update(float deltaTime) override;

    private:
        DirectX::SimpleMath::Vector3 position{};
        DirectX::SimpleMath::Matrix rollMatrix{};
        float radius = 0.5f;
        float speed  = 5.0f;
        DirectX::BoundingSphere collider{};

        // Top surface of KatamariFloorComponent (box hy=0.1 translated to Y=-0.6)
        static constexpr float groundY = -0.5f;

        void CheckCollision();
    };
}

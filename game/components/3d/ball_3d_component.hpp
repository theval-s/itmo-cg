//
// Created by Val on 18.05.2026.
//

#pragma once
#include "math.hpp"
#include "mesh_component.hpp"

namespace val_cg {
    class Ball3DComponent : public MeshComponent {
    public:
        explicit Ball3DComponent(Game *game);

        void Initialize() override;

        void Update(float deltaTime) override;

    private:
        math::Vector3 position;
        math::Vector3 velocity;
        float speed = 3.0f;
        math::BoundingSphere collider;
        static constexpr float radius = 0.15f;
        static constexpr float fieldHalfX = 5.0f;
        static constexpr float fieldHalfY = 3.0f;

        void Reset();
    };
}

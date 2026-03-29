//
// Created by Val on 22.03.2026.
//

#pragma once

#include "d3d11.h"
#include "triangle_component.hpp"
#include "SimpleMath.h"

namespace val_cg {
    class BallComponent final : public TriangleComponent {

    public:
        ID3D11Buffer* constantBuffer;


    public:
        explicit BallComponent(Game* game);

        void Initialize() override;
        void Draw() override;
        //void Reload() override;
        void Update(float deltaTime) override;
        //void DestroyResources() override;

    private:
        struct BallBuffer {
            DirectX::SimpleMath::Vector4 offset;
        };
        BallBuffer data{};

        float speed = 1.f;
        float v1 = 1, v2 = 0;
        DirectX::BoundingBox collider;

    private:
        void ResetBall();
    };
}


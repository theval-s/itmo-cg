//
// Created by Val on 22.03.2026.
//

#pragma once

#include "triangle_component.hpp"
#include "rhi/graphics_device.hpp"
#include "SimpleMath.h"

namespace val_cg {
    class BallComponent final : public TriangleComponent {

    public:
        rhi::GpuBuffer* constantBuffer = nullptr;


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


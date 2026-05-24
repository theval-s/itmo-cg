//
// Created by Val on 22.03.2026.
//

#pragma once

#include "game_component.hpp"
#include "triangle_component.hpp"
#include "d3d11.h"
#include "SimpleMath.h"
#include "DirectXCollision.h"

namespace val_cg {
    class PaddleComponent : public TriangleComponent {

    public:
        enum Alignment : int {
            Left,
            Right,
            Top,
            Bottom
        };
        ID3D11Buffer* constantBuffer = nullptr;
        DirectX::BoundingBox collider;


    private:
        int alignment;
        float speed = 1;

        struct PaddleBuffer {
            DirectX::SimpleMath::Vector4 offset;
        };
        PaddleBuffer data{};

    public:
        PaddleComponent(Game* game, Alignment al);

        void Initialize() override;
        void Draw() override;
        //void Reload() override;
        void Update(float deltaTime) override;
        void DestroyResources() override;
    };

} //namespace val_cg

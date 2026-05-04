//
// Created by Val on 30.03.2026.
//

#pragma once
#include "../camera_component.hpp"
#include "game_component.hpp"
#include "../triangle_component.hpp"

namespace val_cg {
    class MeshComponent : public TriangleComponent {
        public:
        explicit MeshComponent(Game* game, std::span<const DirectX::XMFLOAT4> inputPoints = {}, std::span<const int> inputIndices = {});

        void Initialize() override;
        void Draw() override;
        void Update(float deltaTime) override;
    protected:
        ID3D11Buffer* constantBuffer = nullptr;
        WorldViewProjData data{};
        DirectX::SimpleMath::Matrix worldMatrix{};
    };
}

//
// Created by Val on 30.03.2026.
//

#pragma once
#include "../camera_component.hpp"
#include "game_component.hpp"
#include "../triangle_component.hpp"
#include "rhi/graphics_device.hpp"

namespace val_cg {
    class MeshComponent : public TriangleComponent {
        public:
        explicit MeshComponent(Game* game, std::span<const math::Float4> inputPoints = {}, std::span<const int> inputIndices = {});

        void Initialize() override;
        void Draw() override;
        void Update(float deltaTime) override;
        math::Matrix GetWorldMatrix() const { return worldMatrix; }

    protected:
        rhi::GpuBuffer* constantBuffer = nullptr;
        WorldViewProjData data{};
        math::Matrix worldMatrix{};
        rhi::PrimitiveTopology topology = rhi::PrimitiveTopology::TriangleList;
    };
}

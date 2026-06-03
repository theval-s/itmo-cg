//
// Created by Volkov Sergey on 26/02/2026.
//

#pragma once

#include <directxmath.h>
#include <span>
#include <vector>

#include "../game_component.hpp"
#include "consts.hpp"
#include "common_structs.h"
#include "rhi/graphics_device.hpp"


namespace val_cg
{
    class TriangleComponent : public GameComponent
    {
    protected:
        // Pipeline = shaders + input layout + fixed-function state. Created once
        // (deduplicated in the device's state cache); referenced, not owned.
        rhi::GpuPipeline* pipeline = nullptr;
        rhi::GpuBuffer*   vb        = nullptr;
        rhi::GpuBuffer*   ib        = nullptr;
        std::vector<DirectX::XMFLOAT4> points;
        std::vector<int> indices;
    public:
        ///@param inputPoints vertices in format (xmfloat4)Position (xmfloat4)Color
        explicit TriangleComponent(Game* game, std::span<const DirectX::XMFLOAT4> inputPoints = {}, std::span<const int> inputIndices = {});

        void Initialize() override;
        void Draw() override;
        void DestroyResources() override;
    };
} // val_cg

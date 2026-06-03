//
// Created by Val on 25.05.2026.
//

#pragma once
#include "model_component.hpp"
#include "rhi/graphics_device.hpp"
#include <string>

namespace val_cg {
    // ModelComponent variant that renders with a diffuse texture instead of
    // per-vertex colour. Inherits assimp loading, BoundingSphere, and absorbed
    // logic from ModelComponent; overrides Initialize and Draw for the textured pipeline.
    class TexturedModelComponent : public ModelComponent {
    public:
        TexturedModelComponent(Game* game,
                               const std::string& modelPath,
                               const std::wstring& texturePath,
                               DirectX::SimpleMath::Vector3 position,
                               float scale);

        void Initialize() override;
        void Draw() override;

    private:
        std::wstring texturePath;
        rhi::GpuTexture* srv     = nullptr;
        rhi::GpuSampler* sampler = nullptr;
        int vertexCount = 0;
    };
}

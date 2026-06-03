//
// Created by Val on 25.05.2026.
//

#pragma once
#include "lit_mesh_component.hpp"
#include <DirectXCollision.h>
#include <string>
#include <vector>

namespace val_cg {
    // height-map terrain plane
    class TerrainComponent : public MeshComponent {
    public:
        // heightMapPath : grayscale PNG for elevation (nullptr → procedural)
        // diffusePath   : color bitmap to texture the surface (nullptr → solid green fallback)
        TerrainComponent(Game* game,
            const wchar_t* heightMapPath,
            const wchar_t* diffusePath,
            float width = 30.f, float depth = 30.f,
            float maxHeight = 2.5f);

        void Initialize() override;
        void Draw() override;
        void Update(float deltaTime) override;
        bool IsDeferred() const override { return true; }

        // Called by RenderingSystem geometry pass (terrain G-buffer shaders bound externally).
        void DrawDeferred(rhi::CommandList* cmd, rhi::GpuBuffer* gbufferCB);

        // Returns world-space Y of terrain surface at (wx, wz)
        float GetHeightAt(float wx, float wz) const;

        static constexpr float baseY = -0.6f;
    private:
        int rows_, cols_;
        float width_, depth_, maxHeight_;
        std::vector<float> heights_;
        std::wstring diffusePath_;

        rhi::GpuTexture* srv_     = nullptr;
        rhi::GpuSampler* sampler_ = nullptr;

        static std::vector<float> TryLoadFromFile(const wchar_t* path, int& outRows, int& outCols);
        static std::vector<float> ProceduralHeights(int rows, int cols);
    };

    // ball that acts as a player
    class KatamariPlayerComponent : public LitMeshComponent {
    public:
        explicit KatamariPlayerComponent(Game* game);

        void Initialize() override;
        void Update(float deltaTime) override;
        void Draw() override;
        bool IsTextured() const override { return true; }
        void DrawDeferred(rhi::CommandList* cmd, rhi::GpuBuffer* gbufferCB) override;
        const DirectX::SimpleMath::Vector3& GetPosition() const { return position; }
        const float& GetRadius() const { return radius; }
        void SetTerrain(TerrainComponent* t) { terrain_ = t; }
    private:
        DirectX::SimpleMath::Vector3 position{};
        DirectX::SimpleMath::Quaternion rollRotation{};
        DirectX::SimpleMath::Matrix rollMatrix{};
        float radius = 0.5f;
        float speed  = 5.0f;
        DirectX::BoundingSphere collider{};
        TerrainComponent* terrain_ = nullptr;

        void CheckCollision();

        rhi::GpuTexture* srv_     = nullptr;
        rhi::GpuSampler* sampler_ = nullptr;
    };
}

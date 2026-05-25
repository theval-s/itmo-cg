//
// Created by Val on 25.05.2026.
//

#pragma once
#include "mesh_component.hpp"
#include <DirectXCollision.h>
#include <string>

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
        explicit KatamariPlayerComponent(Game* game, std::wstring pathToTexture);

        void Initialize() override;
        void Update(float deltaTime) override;
        void Draw() override;
        const DirectX::SimpleMath::Vector3& GetPosition() const { return position; }
        const float& GetRadius() const { return radius; }
    private:
        DirectX::SimpleMath::Vector3 position{};
        DirectX::SimpleMath::Quaternion rollRotation{};
        DirectX::SimpleMath::Matrix rollMatrix{};
        float radius = 0.5f;
        float speed  = 5.0f;
        DirectX::BoundingSphere collider{};
        // Top surface of KatamariFloorComponent (box hy=0.1 translated to Y=-0.6)
        static constexpr float groundY = -0.5f;

        void CheckCollision();

        //for texture
        std::wstring texturePath;
        ID3D11ShaderResourceView* srv = nullptr;
        ID3D11SamplerState* sampler   = nullptr;
        int vertexCount = 0;
    };
}

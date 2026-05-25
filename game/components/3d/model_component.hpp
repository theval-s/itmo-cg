//
// Created by Val on 25.05.2026.
//

#pragma once
#include "mesh_component.hpp"
#include <DirectXCollision.h>
#include <string>

namespace val_cg {
    // Loads a mesh from an FBX/OBJ file via Assimp. Supports bounding-sphere
    // collision and an "absorbed" flag used by the Katamari player.
    class ModelComponent : public MeshComponent {
    public:
        ModelComponent(Game* game, const std::string& filePath,
                       DirectX::SimpleMath::Vector3 position,
                       float scale,
                       DirectX::XMFLOAT4 color);

        void Initialize() override;
        void Update(float deltaTime) override;
        void Draw() override;

        bool IsAttached() const { return attachPoint != nullptr; }
        // worldOffset is in world space at the moment of attachment;
        // it is converted to local ball space so it rotates with the ball.
        void AttachTo(const DirectX::SimpleMath::Vector3* posTarget,
                      const DirectX::SimpleMath::Matrix* rotTarget,
                      DirectX::SimpleMath::Vector3 worldOffset) {
            attachPoint    = posTarget;
            attachRotation = rotTarget;
            // store offset in local ball space (rotTarget is orthogonal → transpose = inverse)
            attachOffset = DirectX::SimpleMath::Vector3::TransformNormal(
                worldOffset, rotTarget->Transpose());
        }
        const DirectX::BoundingSphere& GetCollider() const { return collider; }
        float GetObjectRadius() const { return collider.Radius; }

    protected:
        DirectX::BoundingSphere collider{};
        DirectX::SimpleMath::Vector3 worldPos{};
        float scaleVal = 1.0f;
        float baseBoundingRadius = 0.5f;
        const DirectX::SimpleMath::Vector3* attachPoint    = nullptr;
        const DirectX::SimpleMath::Matrix* attachRotation  = nullptr;
        DirectX::SimpleMath::Vector3 attachOffset{};
        std::vector<DirectX::XMFLOAT2> meshUVs;

    private:
        void LoadMesh(const std::string& filePath, DirectX::XMFLOAT4 color);
    };
}

//
// Created by Val on 25.05.2026.
//

#pragma once
#include "math.hpp"
#include "mesh_component.hpp"
#include <string>

namespace val_cg {
    // Loads a mesh from an FBX/OBJ file via Assimp. Supports bounding-sphere
    // collision and an "absorbed" flag used by the Katamari player.
    class ModelComponent : virtual public MeshComponent {
    public:
        ModelComponent(Game* game, const std::string& filePath,
                       math::Vector3 position,
                       float scale,
                       math::Float4 color);

        void Initialize() override;
        void Update(float deltaTime) override;
        void Draw() override;

        bool IsAttached() const { return attachPoint != nullptr; }
        // worldOffset is in world space at the moment of attachment;
        // it is converted to local ball space so it rotates with the ball.
        void AttachTo(const math::Vector3* posTarget,
                      const math::Matrix* rotTarget,
                      math::Vector3 worldOffset) {
            attachPoint    = posTarget;
            attachRotation = rotTarget;
            // store offset in local ball space (rotTarget is orthogonal → transpose = inverse)
            attachOffset = math::Vector3::TransformNormal(
                worldOffset, rotTarget->Transpose());
            // save ball's rotation at attach time so we can apply only the delta each frame
            attachRotationOffset = rotTarget->Transpose();
        }
        const math::BoundingSphere& GetCollider() const { return collider; }
        float GetObjectRadius() const { return collider.Radius; }

    protected:
        math::BoundingSphere collider{};
        math::Vector3 worldPos{};
        float scaleVal = 1.0f;
        float baseBoundingRadius = 0.5f;
        const math::Vector3* attachPoint    = nullptr;
        const math::Matrix* attachRotation  = nullptr;
        math::Vector3 attachOffset{};
        math::Matrix attachRotationOffset{};
        std::vector<math::Float2> meshUVs;
        std::vector<math::Float3> meshNormals;
        std::string modelFilePath;

        void LoadMesh(const std::string& filePath, math::Float4 color);
    };
}

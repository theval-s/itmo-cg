#pragma once
#include "model_component.hpp"
#include "lit_mesh_component.hpp"

namespace val_cg {
    // Phong-lit OBJ model. Inherits geometry/collision from ModelComponent and
    // Phong/shadow CB machinery from LitMeshComponent. The shared MeshComponent
    // base is resolved via virtual inheritance in both parents.
    class LitModelComponent : public LitMeshComponent, public ModelComponent {
    public:
        LitModelComponent(Game* game,
                          const std::string& filePath,
                          DirectX::SimpleMath::Vector3 position,
                          float scale);

        void Initialize() override;
        void Draw() override;
        // DrawDepth() is inherited from LitMeshComponent.
    };
}

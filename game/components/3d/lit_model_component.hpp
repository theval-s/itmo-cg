#pragma once
#include "model_component.hpp"
#include "common_structs.h"

namespace val_cg {
    class LitModelComponent : public ModelComponent {
    public:
        LitModelComponent(Game* game,
                          const std::string& filePath,
                          DirectX::SimpleMath::Vector3 position,
                          float scale);

        void Initialize() override;
        void Draw() override;

    private:
        PhongCBData cbData{};
        ID3D11Buffer* phongCB = nullptr;
    };
}

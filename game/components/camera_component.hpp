//
// Created by Val on 23.03.2026.
//

#pragma once
#include "game_component.hpp"
#include "SimpleMath.h"

namespace val_cg {
    struct CameraData {
        DirectX::SimpleMath::Matrix viewMatrix;
        DirectX::SimpleMath::Matrix projMatrix;
    };

    class CameraComponent: public val_cg::GameComponent {
    public:
        CameraComponent() = delete;
        explicit CameraComponent(Game* game);

        void SetPosition(const DirectX::SimpleMath::Vector3& position);
        void Initialize() override;
        //void Draw() override;  //??? instead we just draw using camera view matrix
        void Update(float deltaTime) override;
        [[nodiscard]] CameraData GetCameraData() const;

    public:
        DirectX::SimpleMath::Matrix viewMatrix;
        DirectX::SimpleMath::Matrix projMatrix;
    private:
        DirectX::SimpleMath::Vector3 cameraPosition;
        DirectX::SimpleMath::Vector3 cameraRotation;
    };
}//namespace val_cg

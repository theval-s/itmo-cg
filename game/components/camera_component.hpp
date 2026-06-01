//
// Created by Val on 23.03.2026.
//

#pragma once
#include "game_component.hpp"
#include "InputDevice.h"
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
        void SetOrbitTarget(const DirectX::SimpleMath::Vector3 *target, const float *radius, float distance);

        //void Draw() override;  //??? instead we just draw using camera view matrix
        void Update(float deltaTime) override;
        [[nodiscard]] CameraData GetCameraData() const;
        [[nodiscard]] DirectX::SimpleMath::Vector3 GetPosition() const { return cameraPosition; }

        void OnMouseMove(const InputDevice::MouseMoveEventArgs& args);

    public:
        DirectX::SimpleMath::Matrix viewMatrix;
        DirectX::SimpleMath::Matrix projMatrix;
    private:
        DirectX::SimpleMath::Vector3 cameraPosition;
        DirectX::SimpleMath::Vector3 cameraRotation;
        const DirectX::SimpleMath::Vector3* orbitTarget = nullptr;
        const float* orbitTargetRadius = nullptr;
        float orbitDistance = 5.f;
    };
}//namespace val_cg

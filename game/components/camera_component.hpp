//
// Created by Val on 23.03.2026.
//

#pragma once
#include "math.hpp"
#include "game_component.hpp"
#include "InputDevice.h"

namespace val_cg {
    struct CameraData {
        math::Matrix viewMatrix;
        math::Matrix projMatrix;
    };

    class CameraComponent: public val_cg::GameComponent {
    public:
        CameraComponent() = delete;
        explicit CameraComponent(Game* game);

        void SetPosition(const math::Vector3& position);
        void Initialize() override;
        void SetOrbitTarget(const math::Vector3 *target, const float *radius, float distance);

        //void Draw() override;  //??? instead we just draw using camera view matrix
        void Update(float deltaTime) override;
        [[nodiscard]] CameraData GetCameraData() const;
        [[nodiscard]] math::Vector3 GetPosition() const { return cameraPosition; }
        [[nodiscard]] math::Vector3 GetForward() const { return forward; }

        void OnMouseMove(const InputDevice::MouseMoveEventArgs& args);

    public:
        math::Matrix viewMatrix;
        math::Matrix projMatrix;
    private:
        math::Vector3 cameraPosition;
        math::Vector3 cameraRotation;
        math::Vector3 forward;
        const math::Vector3* orbitTarget = nullptr;
        const float* orbitTargetRadius = nullptr;
        float orbitDistance = 5.f;
    };
}//namespace val_cg

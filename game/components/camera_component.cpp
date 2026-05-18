//
// Created by Val on 23.03.2026.
//

#include "camera_component.hpp"

#include <iostream>
#include <SimpleMath.h>

#include "game.hpp"

using namespace DirectX::SimpleMath;

val_cg::CameraComponent::CameraComponent(Game *game): GameComponent(game) {
    cameraRotation = Vector3(0.f, 0.f, 0.f);
    cameraPosition = Vector3(0.f, 0.f, 0.f);
    projMatrix = Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PIDIV2, 1.f, 0.001f, 100.0f);
}

void val_cg::CameraComponent::SetPosition(const Vector3 &position) {
    cameraPosition = position;
}

void val_cg::CameraComponent::Initialize() {
    GameComponent::Initialize();
    auto input = game->InputHandler();
    if (input) {
        std::cout << "Added delegate subscription\n";
        input->MouseMove.AddLambda([this](const InputDevice::MouseMoveEventArgs& args) {
            this->OnMouseMove(args);
        });
    }
}

void val_cg::CameraComponent::Update(float deltaTime) {
    auto input = game->InputHandler();
    if (!input) return;

    auto rot = Matrix::CreateFromYawPitchRoll(cameraRotation);

    if (input->IsKeyDown(Keys::W)) {
        //std::cout << "camera forward" << cameraPosition.x << " " <<  cameraPosition.y << " " << cameraPosition.z <<"\n";
        cameraPosition += rot.Forward(); //todo: add modifiable move speed value
    }
    if (input->IsKeyDown(Keys::S)) {
        //std::cout << "camera backward" << cameraPosition.x << " " <<  cameraPosition.y << " " << cameraPosition.z <<"\n";
        cameraPosition -= rot.Forward();
    }
    if (input->IsKeyDown(Keys::A)) cameraPosition -= rot.Right();
    if (input->IsKeyDown(Keys::D)) cameraPosition += rot.Right();
    if (input->IsKeyDown(Keys::Space)) cameraPosition += rot.Up();
    if (input->IsKeyDown(Keys::LeftShift)) cameraPosition -= rot.Up();

    viewMatrix = Matrix::CreateLookAt(cameraPosition, cameraPosition+rot.Forward(),rot.Up());
}

val_cg::CameraData val_cg::CameraComponent::GetCameraData() const {
    return {viewMatrix, projMatrix};
}

void val_cg::CameraComponent::OnMouseMove(const InputDevice::MouseMoveEventArgs &args) {
    float mouseSpeed = 0.001f;
    //yes, y-x and x-y is right
    cameraRotation.y -= args.Offset.x * mouseSpeed;
    cameraRotation.x -= args.Offset.y * mouseSpeed;
}

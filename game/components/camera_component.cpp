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
}

void val_cg::CameraComponent::Update(float deltaTime) {
    auto input = game->InputHandler();
    if (!input) return;

    cameraRotation.y -= input->MouseOffset.x * 0.005f; //todo: extract mouse speed value
    cameraRotation.x -= input->MouseOffset.y * 0.005f;
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

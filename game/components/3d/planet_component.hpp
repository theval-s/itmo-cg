//
// Created by Val on 7.04.2026.
//

#pragma once
#include "mesh_component.hpp"

namespace val_cg {

    class PlanetComponent;
    class OrbitComponent : public MeshComponent {
    public:
        OrbitComponent(Game* game, PlanetComponent* parent, int segmentCount, float orbitRadius, DirectX::XMFLOAT4 color);
        void Update(float deltaTime) override;

    private:
        PlanetComponent* parent = nullptr;
    };
    class PlanetComponent final : public MeshComponent {
        friend class OrbitComponent;
    public:
        PlanetComponent(Game* game,
                        float orbitRadius,
                        float orbitSpeed,
                        float rotationSpeed,
                        float scale,
                        DirectX::XMFLOAT4 color,
                        PlanetComponent* parent = nullptr);

        void Update(float deltaTime) override;
        void MakeLineList(const DirectX::XMFLOAT4& color);
        void MakeTriangleList(const DirectX::XMFLOAT4& color);
        void MakeOrbit();
        void DestroyResources() override;
        void Draw() override;
        DirectX::SimpleMath::Matrix GetWorldMatrix() {
            return worldMatrix;
        };

    private:
        float orbitRadius = 0.f;
        float orbitSpeed = 0.f;
        float rotationSpeed = 0.f;
        float scale = 0.f;
        float currentOrbitAngle = 0.0f;
        float currentRotationAngle = 0.0f;
        PlanetComponent* parent = nullptr;

        OrbitComponent* orbit = nullptr;
    };
}

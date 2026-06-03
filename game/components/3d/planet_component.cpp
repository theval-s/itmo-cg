//
// Created by Val on 7.04.2026.
//
#include "planet_component.hpp"

#include <iostream>

#include "game.hpp"
#include "utils/geometry_generator.hpp"
#include "SimpleMath.h"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    OrbitComponent::OrbitComponent(Game* game, PlanetComponent *parent, int segmentCount, float orbitRadius,
        DirectX::XMFLOAT4 color): MeshComponent(game), parent(parent) {
        topology = rhi::PrimitiveTopology::LineList;
        points.clear();
        indices.clear();

        //float phiStep = DirectX::XM_PI / segmentCount;
        const float thetaStep = DirectX::XM_2PI / segmentCount;
        for (int i = 0; i < segmentCount; i++) {
            const float theta = i * thetaStep;
            const float x = orbitRadius * cosf(theta);
            const float z = orbitRadius * sinf(theta);
            points.push_back({x, 0.f, z, 1.f});
            points.push_back(color);
            indices.push_back(i);
            indices.push_back((i + 1) % segmentCount);
        }
    }

    void OrbitComponent::Update(float deltaTime) {
        if (parent && parent->parent) {
            const Vector3 grandparentPos = parent->parent->worldMatrix.Translation();
            worldMatrix = Matrix::CreateTranslation(grandparentPos);
        } else {
            worldMatrix = Matrix::Identity;
        }
        MeshComponent::Update(deltaTime);
    }

    PlanetComponent::PlanetComponent(Game* game,
                                     float orbitRadius,
                                     float orbitSpeed,
                                     float rotationSpeed,
                                     float scale,
                                     DirectX::XMFLOAT4 color,
                                     PlanetComponent* parent
    )
        : MeshComponent(game),
          orbitRadius(orbitRadius),
          orbitSpeed(orbitSpeed),
          rotationSpeed(rotationSpeed),
          scale(scale),
          parent(parent)
    {
        MakeTriangleList(color);
    }

    void PlanetComponent::Update(float deltaTime) {
        currentOrbitAngle += orbitSpeed * deltaTime;
        currentRotationAngle += rotationSpeed * deltaTime;

        Matrix rotation = Matrix::CreateRotationY(currentRotationAngle);
        Matrix scaling = Matrix::CreateScale(scale);
        Matrix translation = Matrix::CreateTranslation(orbitRadius * cos(currentOrbitAngle), 0, orbitRadius * sin(currentOrbitAngle));
        //std::cout << "rotation " << currentRotationAngle << " -> " << currentOrbitAngle << std::endl;
        worldMatrix = scaling * rotation * translation;

        if (parent) {
            //std::cout << "I have parent\n";
            //handling moon behavior by modifying world matrix
            Vector3 parentPos = parent->worldMatrix.Translation();
            worldMatrix *= Matrix::CreateTranslation(parentPos);
        }
        //std::cout << "translation: " << worldMatrix.Translation().x << " " << worldMatrix.Translation().y << " " << worldMatrix.Translation().z << std::endl;


        //updating constant buffer
        MeshComponent::Update(deltaTime);

        if (orbit) orbit->Update(deltaTime);
    }

    void PlanetComponent::MakeLineList(const DirectX::XMFLOAT4& color) {
        MeshData mesh = GeometryGenerator::CreateSphereLineList((parent)?0.1f:0.5f,10,10, color);
        topology = rhi::PrimitiveTopology::LineList;

        //todo: change the whole structure
        std::vector<DirectX::XMFLOAT4> tempPoints;
        tempPoints.reserve(mesh.vertices.size()*2);
        for (const auto& v : mesh.vertices) {
            tempPoints.push_back(v.position);
            tempPoints.push_back(v.color);
        }
        this->points = tempPoints;
        this->indices = mesh.indices;
    }

    void PlanetComponent::MakeTriangleList(const DirectX::XMFLOAT4& color) {
        MeshData mesh = GeometryGenerator::CreateSphere((parent)?0.1f:0.5f,10,10, color);
        topology = rhi::PrimitiveTopology::TriangleList;

        //todo: change the whole structure
        std::vector<DirectX::XMFLOAT4> tempPoints;
        tempPoints.reserve(mesh.vertices.size()*2);
        for (const auto& v : mesh.vertices) {
            tempPoints.push_back(v.position);
            tempPoints.push_back(v.color);
        }
        this->points = tempPoints;
        this->indices = mesh.indices;
    }

    void PlanetComponent::MakeOrbit() {
        orbit = new OrbitComponent(game, this, 64, orbitRadius, {0.62f, 0.62f, 0.62f,1});
        orbit->Initialize();
    }

    void PlanetComponent::DestroyResources() {
        MeshComponent::DestroyResources();

        if (orbit) {
            orbit->DestroyResources();
            delete orbit;
        }
    }

    void PlanetComponent::Draw() {
        MeshComponent::Draw();

        if (orbit) {
            orbit->Draw();
        }
    }
}


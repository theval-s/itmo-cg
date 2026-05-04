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
        //todo: random radius, slices?
        MeshData mesh = GeometryGenerator::CreateSphere((parent)?0.1f:0.5f,10,10, color);

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

    void PlanetComponent::Update(float deltaTime) {
        currentOrbitAngle += orbitSpeed * deltaTime;
        currentRotationAngle += rotationSpeed * deltaTime;

        //Matrix rotation = Matrix::CreateRotationY(currentRotationAngle);
        Matrix rotation = {};
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
    }
}


//
// Created by Val on 25.05.2026.
//

#include "model_component.hpp"
#include "game.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    ModelComponent::ModelComponent(Game* game, const std::string& filePath,
                                   Vector3 position, float scale, DirectX::XMFLOAT4 color)
        : MeshComponent(game), worldPos(position), scaleVal(scale)
    {
        LoadMesh(filePath, color);
    }

    void ModelComponent::LoadMesh(const std::string& filePath, DirectX::XMFLOAT4 color) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filePath,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_PreTransformVertices |
            aiProcess_FlipUVs);

        if (!scene || !scene->HasMeshes()) {
            std::cerr << "[ModelComponent] Failed to load: " << filePath
                      << " — " << importer.GetErrorString() << "\n";
            return; // TriangleComponent left default quad geometry as fallback
        }

        points.clear();
        indices.clear();
        meshUVs.clear();

        std::vector<DirectX::XMFLOAT3> allPositions;
        int vertexOffset = 0;

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
            const aiMesh* mesh = scene->mMeshes[m];

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                const auto& v = mesh->mVertices[i];
                allPositions.push_back({v.x, v.y, v.z});
                points.push_back({v.x, v.y, v.z, 1.0f});
                points.push_back(color);

                if (mesh->HasTextureCoords(0)) {
                    const auto& tc = mesh->mTextureCoords[0][i];
                    meshUVs.emplace_back(tc.x, tc.y);
                } else {
                    meshUVs.emplace_back(0.f, 0.f);
                }
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                const aiFace& face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; ++j)
                    indices.push_back(static_cast<int>(vertexOffset + face.mIndices[j]));
            }

            vertexOffset += static_cast<int>(mesh->mNumVertices);
        }

        if (!allPositions.empty()) {
            DirectX::BoundingSphere::CreateFromPoints(
                collider, allPositions.size(),
                allPositions.data(), sizeof(DirectX::XMFLOAT3));
            baseBoundingRadius = collider.Radius;
        }
    }

    void ModelComponent::Initialize() {
        MeshComponent::Initialize();
        collider.Radius = baseBoundingRadius * scaleVal;
        collider.Center = {worldPos.x, worldPos.y, worldPos.z};
    }

    void ModelComponent::Update(float deltaTime) {
        if (attachPoint)
            worldPos = *attachPoint +
                DirectX::SimpleMath::Vector3::TransformNormal(attachOffset, *attachRotation);
        worldMatrix = Matrix::CreateScale(scaleVal) * Matrix::CreateTranslation(worldPos);
        collider.Center = {worldPos.x, worldPos.y, worldPos.z};
        MeshComponent::Update(deltaTime);
    }

    void ModelComponent::Draw() {
        MeshComponent::Draw();
    }
}

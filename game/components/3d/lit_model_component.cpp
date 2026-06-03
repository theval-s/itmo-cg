#include "lit_model_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <algorithm>
#include <iostream>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    LitModelComponent::LitModelComponent(Game* game,
                                         const std::string& filePath,
                                         Vector3 position,
                                         float scale)
        : MeshComponent(game)   // explicit virtual-base init
        , LitMeshComponent(game)
        , ModelComponent(game, filePath, position, scale, {0.8f, 0.8f, 0.8f, 1.f})
    {}

    void LitModelComponent::Initialize() {
        auto* dev = game->GetDevice();

        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(PHONG_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = dev->CreateShader(PHONG_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
        pd.layout.attributes = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"NORMAL",   0, rhi::VertexFormat::Float4},
        };
        pd.raster.cull = rhi::CullMode::None;
        pipeline = dev->CreatePipeline(pd);

        // Build PhongVertex buffer from loaded positions and normals.
        int vertexCount = static_cast<int>(points.size()) / 2;
        std::vector<PhongVertex> phongVerts;
        phongVerts.reserve(vertexCount);
        for (int i = 0; i < vertexCount; ++i) {
            PhongVertex pv;
            pv.position = points[i * 2];
            const auto& n = (i < static_cast<int>(meshNormals.size()))
                            ? meshNormals[i]
                            : DirectX::XMFLOAT3{0.f, 1.f, 0.f};
            pv.normal = {n.x, n.y, n.z, 0.f};
            phongVerts.push_back(pv);
        }

        vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(PhongVertex) * phongVerts.size()}, phongVerts.data());
        ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},           indices.data());

        InitLitBuffers();

        // Read material properties from MTL via Assimp.
        aiColor3D ka(0.1f, 0.1f, 0.1f);
        aiColor3D kd(0.8f, 0.8f, 0.8f);
        aiColor3D ks(1.0f, 1.0f, 1.0f);
        float ns = 32.f;
        {
            Assimp::Importer imp;
            const aiScene* s = imp.ReadFile(modelFilePath, 0);
            if (s && s->mNumMaterials > 0) {
                aiMaterial* mat = s->mMaterials[0];
                mat->Get(AI_MATKEY_COLOR_AMBIENT,  ka);
                mat->Get(AI_MATKEY_COLOR_DIFFUSE,  kd);
                mat->Get(AI_MATKEY_COLOR_SPECULAR, ks);
                mat->Get(AI_MATKEY_SHININESS,      ns);
            }
        }
        cbData.matAmbient  = {ka.r, ka.g, ka.b, 0.f};
        cbData.matDiffuse  = {kd.r, kd.g, kd.b, 0.f};
        cbData.matSpecular = {ks.r, ks.g, ks.b, ns};

        collider.Radius = baseBoundingRadius * scaleVal;
        collider.Center = {worldPos.x, worldPos.y, worldPos.z};
    }

    void LitModelComponent::Draw() {
        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(pipeline);
        cmd->SetVertexBuffer(vb, sizeof(PhongVertex));
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);

        BindPhongCB(worldMatrix);
        BindShadow();

        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }
}

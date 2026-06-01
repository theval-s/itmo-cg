#include "lit_model_component.hpp"
#include "lights/light_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <iostream>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    LitModelComponent::LitModelComponent(Game* game,
                                         const std::string& filePath,
                                         Vector3 position,
                                         float scale)
        : ModelComponent(game, filePath, position, scale, {0.8f, 0.8f, 0.8f, 1.f})
    {}

    void LitModelComponent::Initialize() {
        ID3DBlob* errorCode = nullptr;

        HRESULT res = D3DCompileFromFile(PHONG_SHADER_PATH, nullptr, nullptr,
            "VSMain", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
            &vertexShaderByteCode, &errorCode);
        if (FAILED(res)) {
            if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
            else throw std::runtime_error("PhongShader.hlsl not found");
        }

        res = D3DCompileFromFile(PHONG_SHADER_PATH, nullptr, nullptr,
            "PSMain", "ps_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
            &pixelShaderByteCode, &errorCode);
        if (FAILED(res)) {
            if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
            else throw std::runtime_error("PhongShader.hlsl not found");
        }

        game->renderer.device->CreateVertexShader(
            vertexShaderByteCode->GetBufferPointer(),
            vertexShaderByteCode->GetBufferSize(), nullptr, &vertexShader);
        game->renderer.device->CreatePixelShader(
            pixelShaderByteCode->GetBufferPointer(),
            pixelShaderByteCode->GetBufferSize(), nullptr, &pixelShader);

        D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
             D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        game->renderer.device->CreateInputLayout(inputElements, 2,
            vertexShaderByteCode->GetBufferPointer(),
            vertexShaderByteCode->GetBufferSize(), &layout);

        // Build PhongVertex buffer: points[] is interleaved {pos, color} pairs
        int vertexCount = static_cast<int>(points.size()) / 2;
        std::vector<PhongVertex> phongVerts;
        phongVerts.reserve(vertexCount);
        for (int i = 0; i < vertexCount; ++i) {
            PhongVertex pv;
            pv.position = points[i * 2]; // even = position
            const auto& n = (i < static_cast<int>(meshNormals.size()))
                            ? meshNormals[i]
                            : DirectX::XMFLOAT3{0.f, 1.f, 0.f};
            pv.normal = {n.x, n.y, n.z, 0.f};
            phongVerts.push_back(pv);
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage     = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(PhongVertex) * phongVerts.size());
        D3D11_SUBRESOURCE_DATA vbData = {phongVerts.data()};
        game->renderer.device->CreateBuffer(&vbDesc, &vbData, &vb);

        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage     = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(int) * indices.size());
        D3D11_SUBRESOURCE_DATA ibData = {indices.data()};
        game->renderer.device->CreateBuffer(&ibDesc, &ibData, &ib);

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage          = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.ByteWidth      = sizeof(PhongCBData);
        game->renderer.device->CreateBuffer(&cbDesc, nullptr, &phongCB);

        CD3D11_RASTERIZER_DESC rastDesc = {};
        rastDesc.CullMode = D3D11_CULL_NONE;
        rastDesc.FillMode = D3D11_FILL_SOLID;
        game->renderer.device->CreateRasterizerState(&rastDesc, &rastState);

        // Read MTL material via a quick Assimp reload (no geometry processing needed)
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
        game->renderer.deviceContext->RSSetState(rastState);

        D3D11_VIEWPORT viewport = {};
        viewport.Width    = static_cast<float>(game->renderer.ScreenWidth);
        viewport.Height   = static_cast<float>(game->renderer.ScreenHeight);
        viewport.MaxDepth = 1.0f;
        game->renderer.deviceContext->RSSetViewports(1, &viewport);

        game->renderer.deviceContext->IASetInputLayout(layout);
        game->renderer.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        constexpr UINT stride = sizeof(PhongVertex); // 32
        constexpr UINT offset = 0;
        game->renderer.deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        game->renderer.deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

        game->renderer.deviceContext->VSSetShader(vertexShader, nullptr, 0);
        game->renderer.deviceContext->PSSetShader(pixelShader, nullptr, 0);

        const auto camData = game->GetCameraData();
        cbData.worldViewProj = (worldMatrix * camData.viewMatrix * camData.projMatrix).Transpose();
        cbData.world         = worldMatrix.Transpose();

        Vector3 cp = game->GetCamera()->GetPosition();
        cbData.cameraPos = {cp.x, cp.y, cp.z, 0.f};

        const auto& lights = game->GetLights();
        cbData.lightCount = static_cast<int>(std::min(lights.size(), static_cast<size_t>(MAX_LIGHTS)));
        for (int i = 0; i < cbData.lightCount; ++i)
            if (lights[i]->active)
                cbData.lights[i] = lights[i]->GetLightData();

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        game->renderer.deviceContext->Map(phongCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &cbData, sizeof(PhongCBData));
        game->renderer.deviceContext->Unmap(phongCB, 0);
        game->renderer.deviceContext->VSSetConstantBuffers(0, 1, &phongCB);
        game->renderer.deviceContext->PSSetConstantBuffers(0, 1, &phongCB);

        game->renderer.deviceContext->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    }
}

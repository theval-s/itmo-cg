//
// Created by Val on 25.05.2026.
//

#include "katamari_player_component.hpp"
#include "model_component.hpp"
#include "game.hpp"
#include "utils/geometry_generator.hpp"
#include <WICTextureLoader.h>
#include <d3dcompiler.h>
#include <iostream>
#include "Keys.h"
#include <cmath>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    KatamariFloorComponent::KatamariFloorComponent(Game* game) : MeshComponent(game) {
        //plane
        points.clear();
        indices.clear();
        MeshData mesh = GeometryGenerator::CreateBox(30.f, 0.2f, 30.f, {0.3f, 0.55f, 0.25f, 1.f});
        for (const auto& v : mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color);
        }
        indices = mesh.indices;
        worldMatrix = Matrix::CreateTranslation(0.f, -0.6f, 0.f);
    }

    void KatamariFloorComponent::Update(float deltaTime) {
        MeshComponent::Update(deltaTime);
    }

    KatamariPlayerComponent::KatamariPlayerComponent(Game* game, std::wstring pathToTexture)
        : MeshComponent(game), texturePath(pathToTexture)
    {
        //sphere
        points.clear();
        indices.clear();
        MeshData mesh = GeometryGenerator::CreateSphere(1.0f, 16, 16, {0.9f, 0.3f, 0.1f, 1.f});
        for (const auto& v : mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color);
        }
        indices = mesh.indices;
    }

    void KatamariPlayerComponent::Initialize() {
        ID3DBlob* errorCode = nullptr;

        // Vertex shader
        HRESULT res = D3DCompileFromFile(SIMPLE_TEXTURED_SHADER_PATH, nullptr, nullptr,
            "VSMain", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
            &vertexShaderByteCode, &errorCode);
        if (FAILED(res)) {
            if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
            else throw std::runtime_error("TexturedShader.hlsl not found");
        }

        // Pixel shader
        res = D3DCompileFromFile(SIMPLE_TEXTURED_SHADER_PATH, nullptr, nullptr,
            "PSMain", "ps_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
            &pixelShaderByteCode, &errorCode);
        if (FAILED(res)) {
            if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
            else throw std::runtime_error("TexturedShader.hlsl not found");
        }

        game->renderer.device->CreateVertexShader(
            vertexShaderByteCode->GetBufferPointer(),
            vertexShaderByteCode->GetBufferSize(), nullptr, &vertexShader);
        game->renderer.device->CreatePixelShader(
            pixelShaderByteCode->GetBufferPointer(),
            pixelShaderByteCode->GetBufferSize(), nullptr, &pixelShader);

        D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            D3D11_INPUT_ELEMENT_DESC{
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                0,
                0,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            D3D11_INPUT_ELEMENT_DESC{
                "COLOR",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                0,
                D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };

        game->renderer.device->CreateInputLayout(inputElements, 2,
        vertexShaderByteCode->GetBufferPointer(),
        vertexShaderByteCode->GetBufferSize(), &layout);

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage     = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(DirectX::XMFLOAT4) * points.size());
        D3D11_SUBRESOURCE_DATA vbData = { points.data() };
        game->renderer.device->CreateBuffer(&vbDesc, &vbData, &vb);

        // Constant buffer (WorldViewProjData)
        D3D11_BUFFER_DESC constBufDesc = {};
        constBufDesc.Usage = D3D11_USAGE_DYNAMIC;
        constBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constBufDesc.ByteWidth = sizeof(WorldViewProjData);
        game->renderer.device->CreateBuffer(&constBufDesc, nullptr, &constantBuffer);

        // Index buffer
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(int) * indices.size());
        D3D11_SUBRESOURCE_DATA ibData = {indices.data()};
        game->renderer.device->CreateBuffer(&ibDesc, &ibData, &ib);

        // Rasterizer state
        CD3D11_RASTERIZER_DESC rastDesc = {};
        rastDesc.CullMode = D3D11_CULL_NONE;
        rastDesc.FillMode = D3D11_FILL_SOLID;
        game->renderer.device->CreateRasterizerState(&rastDesc, &rastState);

        // Texture
        res = DirectX::CreateWICTextureFromFile(
            game->renderer.device.Get(), texturePath.c_str(), nullptr, &srv);
        if (FAILED(res))
            std::wcerr << L"[TexturedModelComponent] Failed to load texture: "
                       << texturePath << L"\n";

        // Sampler
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
        game->renderer.device->CreateSamplerState(&sampDesc, &sampler);

        rollRotation = Quaternion::Identity;
        rollMatrix = Matrix::Identity;
        position.y = groundY + radius;
        collider.Center = {position.x, position.y, position.z};
        collider.Radius = radius;
    }

    void KatamariPlayerComponent::Update(float deltaTime) {
        auto* input = game->InputHandler();
        Vector3 moveDir = Vector3::Zero;
        if (input) {
            if (input->IsKeyDown(Keys::Up))    moveDir.z += 1.f;
            if (input->IsKeyDown(Keys::Down))  moveDir.z -= 1.f;
            if (input->IsKeyDown(Keys::Left))  moveDir.x += 1.f;
            if (input->IsKeyDown(Keys::Right)) moveDir.x -= 1.f;
        }

        if (moveDir.LengthSquared() > 0.f) {
            moveDir.Normalize();
            float dist = speed * deltaTime;
            position += moveDir * dist;

            //keep rolling rolling rolling
            float rollAngle = dist / radius;

            Vector3 rollAxis = Vector3::Up.Cross(moveDir);

            if (rollAxis.LengthSquared() > 0.f)
            {
                rollAxis.Normalize();

                Quaternion deltaRot =
                    Quaternion::CreateFromAxisAngle(rollAxis, rollAngle);

                rollRotation = rollRotation* deltaRot;
                rollRotation.Normalize();

                rollMatrix = Matrix::CreateFromQuaternion(rollRotation);
            }
        }

        CheckCollision();

        collider.Center = {position.x, position.y, position.z};
        collider.Radius = radius;

        worldMatrix = Matrix::CreateScale(radius) * rollMatrix * Matrix::CreateTranslation(position);
        MeshComponent::Update(deltaTime);
    }

    void KatamariPlayerComponent::Draw() {
        game->renderer.deviceContext->RSSetState(rastState);
        const UINT strides[] = {32}; //float4 + float4
        const UINT offsets[] = {0};

        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(game->renderer.ScreenWidth);
        viewport.Height = static_cast<float>(game->renderer.ScreenHeight);
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1.0f;

        game->renderer.deviceContext->RSSetViewports(1, &viewport);

        game->renderer.deviceContext->IASetInputLayout(layout);
        game->renderer.deviceContext->IASetPrimitiveTopology(topology);
        game->renderer.deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        game->renderer.deviceContext->IASetVertexBuffers(0, 1, &vb, strides, offsets);
        game->renderer.deviceContext->VSSetShader(vertexShader, nullptr, 0);
        game->renderer.deviceContext->PSSetShader(pixelShader, nullptr, 0);

        D3D11_MAPPED_SUBRESOURCE res = {};
        game->renderer.deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
        auto dataPtr = reinterpret_cast<float*>(res.pData);
        memcpy(dataPtr, &data, sizeof(WorldViewProjData));
        game->renderer.deviceContext->Unmap(constantBuffer, 0);
        game->renderer.deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

        game->renderer.deviceContext->PSSetShaderResources(0, 1, &srv);
        game->renderer.deviceContext->PSSetSamplers(0, 1, &sampler);
        game->renderer.deviceContext->DrawIndexed(indices.size(),0,0);
    }

    void KatamariPlayerComponent::CheckCollision() {
        for (auto* comp : game->Components) {
            auto* model = dynamic_cast<ModelComponent*>(comp);
            if (!model || model->IsAttached()) continue;

            if (collider.Intersects(model->GetCollider())) {
                if (model->GetObjectRadius() <= radius * 1.5f) {
                    const auto& c = model->GetCollider().Center;
                    Vector3 worldOffset = Vector3(c.x, c.y, c.z) - position;
                    model->AttachTo(&position, &rollMatrix, worldOffset);

                    // volume-preserving growth: V_new = V_ball + V_model
                    // float r3  = radius * radius * radius;
                    // float mr3 = model->GetObjectRadius() * model->GetObjectRadius() * model->GetObjectRadius();
                    // radius = std::cbrt(r3 + mr3);
                    radius *=1.1f;

                    //raise ball when it grows
                    position.y = groundY + radius;
                }
            }
        }
    }
}

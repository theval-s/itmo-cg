//
// Created by Val on 22.03.2026.
//

#include "paddle_component.hpp"

#include <iostream>
#include <algorithm>
#include "directxmath.h"
#include "d3dcompiler.h"
#include "../game.hpp"
#include "consts.hpp"
#include "Keys.h"


constexpr DirectX::XMFLOAT4 paddle_polys[8] = {
    DirectX::XMFLOAT4(0.05f, 0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
    DirectX::XMFLOAT4(-0.05f, -0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
    DirectX::XMFLOAT4(0.05f, -0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
    DirectX::XMFLOAT4(-0.05f, 0.2f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
};
constexpr int paddle_indices[6] = {0, 1, 2, 1, 0, 3};


val_cg::PaddleComponent::PaddleComponent(Game *game, Alignment al)
    : TriangleComponent(game, paddle_polys, paddle_indices)
      , alignment(al) {
    if (alignment == Left) {
        data.offset.x = -0.9;
    } else if (alignment == Right) {
        data.offset.x = 0.9;
    } else {
        //throw std::runtime_error("Alignment not supported");
        alignment=Left;
        data.offset.x = -0.9;
    }
}

void val_cg::PaddleComponent::Initialize() {
    ID3DBlob *errorCode = nullptr;

    //Vertex Shader
    HRESULT res = D3DCompileFromFile(PADDLE_SHADER_PATH, nullptr, nullptr, "VSMain", "vs_5_0",
                                     D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShaderByteCode,
                                     &errorCode);
    if (FAILED(res)) {
        if (errorCode) {
            char *compileErrors = (char *) errorCode->GetBufferPointer();
            std::cerr << compileErrors << std::endl;
            errorCode->Release();
        } else {
            throw std::invalid_argument("Vertex shader file not found");
        }
    }
    res = D3DCompileFromFile(PADDLE_SHADER_PATH, nullptr /*Shader_Macros*/, nullptr, "PSMain", "ps_5_0",
                             D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShaderByteCode, &errorCode);
    if (FAILED(res)) {
        if (errorCode) {
            char *compileErrors = (char *) errorCode->GetBufferPointer();
            std::cerr << compileErrors << std::endl;
            errorCode->Release();
        } else {
            throw std::invalid_argument("Pixel shader file not found");
        }
    }

    game->renderer.device->CreateVertexShader(vertexShaderByteCode->GetBufferPointer(),
                                              vertexShaderByteCode->GetBufferSize(),
                                              nullptr, &vertexShader);
    game->renderer.device->CreatePixelShader(pixelShaderByteCode->GetBufferPointer(),
                                             pixelShaderByteCode->GetBufferSize(),
                                             nullptr, &pixelShader);

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

    game->renderer.device->CreateInputLayout(inputElements, 2, vertexShaderByteCode->GetBufferPointer(),
                                             vertexShaderByteCode->GetBufferSize(), &layout);

    D3D11_BUFFER_DESC constBufDesc;
    constBufDesc.Usage = D3D11_USAGE_DYNAMIC;
    constBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constBufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constBufDesc.MiscFlags = 0;
    constBufDesc.StructureByteStride = 0;
    constBufDesc.ByteWidth = sizeof(PaddleBuffer);
    game->renderer.device->CreateBuffer(&constBufDesc, nullptr, &constantBuffer);

    D3D11_BUFFER_DESC vertexBufDesc = {};
    vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufDesc.CPUAccessFlags = 0;
    vertexBufDesc.MiscFlags = 0;
    vertexBufDesc.StructureByteStride = 0;
    vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(points);

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = points.data();
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    game->renderer.device->CreateBuffer(&vertexBufDesc, &vertexData, &vb);

    D3D11_BUFFER_DESC indexBufDesc = {};
    indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufDesc.CPUAccessFlags = 0;
    indexBufDesc.MiscFlags = 0;
    indexBufDesc.StructureByteStride = 0;
    indexBufDesc.ByteWidth = sizeof(int) * std::size(indices);

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    game->renderer.device->CreateBuffer(&indexBufDesc, &indexData, &ib);

    CD3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.FillMode = D3D11_FILL_SOLID;

    res = game->renderer.device->CreateRasterizerState(&rastDesc, &rastState);
    if (FAILED(res)) {
        if (errorCode) {
            char *compileErrors = (char *) errorCode->GetBufferPointer();
            std::cerr << compileErrors << std::endl;
            errorCode->Release();
        }
    }

    collider.Center.x=data.offset.x;
    collider.Center.y=0;
    collider.Extents.x=0.05f;
    collider.Extents.y=0.2f;
}

void val_cg::PaddleComponent::Draw() {
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
    game->renderer.deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    game->renderer.deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    game->renderer.deviceContext->IASetVertexBuffers(0, 1, &vb, strides, offsets);
    game->renderer.deviceContext->VSSetShader(vertexShader, nullptr, 0);
    game->renderer.deviceContext->PSSetShader(pixelShader, nullptr, 0);

    D3D11_MAPPED_SUBRESOURCE res = {};
    game->renderer.deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
    auto dataPtr = reinterpret_cast<float*>(res.pData);
    memcpy(dataPtr, &data, sizeof(PaddleBuffer));
    game->renderer.deviceContext->Unmap(constantBuffer, 0);
    game->renderer.deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

    game->renderer.deviceContext->DrawIndexed(6,0,0);
}

// void val_cg::PaddleComponent::Reload() {
//     TriangleComponent::Reload();
// }

void val_cg::PaddleComponent::Update(float deltaTime) {
    if (alignment == Left) {
        // if (game->InputHandler()->IsKeyDown(Keys::W)) {
        //     data.offset.y += speed * deltaTime;
        // } else if (game->InputHandler()->IsKeyDown(Keys::S)) {
        //     data.offset.y -= speed * deltaTime;
        // }
        if (GetAsyncKeyState(static_cast<int>(Keys::S))) {
            data.offset.y = std::clamp(data.offset.y - speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y - speed * deltaTime, -1.f, 1.f);
        } else if (GetAsyncKeyState(static_cast<int>(Keys::W))) {
            data.offset.y = std::clamp(data.offset.y + speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y + speed * deltaTime, -1.f, 1.f);
        }
    } else if (alignment == Right) {
        if (GetAsyncKeyState(static_cast<int>(Keys::Down))) {
            data.offset.y = std::clamp(data.offset.y - speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y - speed * deltaTime, -1.f, 1.f);
        } else if (GetAsyncKeyState(static_cast<int>(Keys::Up))) {
            data.offset.y = std::clamp(data.offset.y + speed * deltaTime, -1.f, 1.f);
            collider.Center.y = std::clamp(collider.Center.y + speed * deltaTime, -1.f, 1.f);
        }
    } else {
        throw std::runtime_error("Alignment not supported");
    }
}

void val_cg::PaddleComponent::DestroyResources() {
    constantBuffer->Release();
    TriangleComponent::DestroyResources();
}

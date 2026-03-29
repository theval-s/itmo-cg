//
// Created by Volkov Sergey on 26/02/2026.
//

#include "triangle_component.hpp"
#include "../consts.hpp"

#include <d3dcompiler.h>
#include <directxmath.h>
#include <iostream>

#include "../game.hpp"


namespace val_cg {
    TriangleComponent::TriangleComponent(Game* game, std::span<const DirectX::XMFLOAT4> inputPoints, std::span<const int> inputIndices)
    : GameComponent(game) {
        if (inputPoints.empty() || inputIndices.empty()) {
            points = {
                //position, color
                DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
                DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
                DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
                DirectX::XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
            };
            indices = {0,1,2, 1,0,3};
        }
        else {
            points.assign_range(inputPoints);
            indices.assign_range(inputIndices);
        }
    }

    void TriangleComponent::Initialize() {
        ID3DBlob *errorCode = nullptr;

        //Vertex Shader
        HRESULT res = D3DCompileFromFile(VERTEX_SHADER_PATH, nullptr, nullptr, "VSMain", "vs_5_0",
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


        //Pixel Shader
        constexpr D3D_SHADER_MACRO Shader_Macros[] = {
            "TEST", "1", "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr
        };
        res = D3DCompileFromFile(PIXEL_SHADER_PATH, nullptr /*Shader_Macros*/, nullptr, "PSMain", "ps_5_0",
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
    }

    void TriangleComponent::Draw() {
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

        game->renderer.deviceContext->DrawIndexed(6,0,0);
    }

    // void TriangleComponent::Reload() {
    //     GameComponent::Reload();
    // }
    //
    // void TriangleComponent::Update(float deltaTime) {
    //     GameComponent::Update(float deltaTime);
    // }

    void TriangleComponent::DestroyResources() {
        layout->Release();
        vertexShader->Release();
        vertexShaderByteCode->Release();
        rastState->Release();
        pixelShader->Release();
        pixelShaderByteCode->Release();

        vb->Release();
        ib->Release();
    }
} // val_cg

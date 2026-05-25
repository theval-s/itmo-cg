//
// Created by Val on 25.05.2026.
//

#include "textured_model_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include <WICTextureLoader.h>
#include <d3dcompiler.h>
#include <iostream>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    TexturedModelComponent::TexturedModelComponent(Game* game,
                                                   const std::string& modelPath,
                                                   const std::wstring& texturePath,
                                                   Vector3 position,
                                                   float scale)
        : ModelComponent(game, modelPath, position, scale, {1.f, 1.f, 1.f, 1.f})
        , texturePath(texturePath)
    {}

    void TexturedModelComponent::Initialize() {
        ID3DBlob* errorCode = nullptr;

        // Vertex shader
        HRESULT res = D3DCompileFromFile(TEXTURED_SHADER_PATH, nullptr, nullptr,
            "VSMain", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
            &vertexShaderByteCode, &errorCode);
        if (FAILED(res)) {
            if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
            else throw std::runtime_error("TexturedShader.hlsl not found");
        }

        // Pixel shader
        res = D3DCompileFromFile(TEXTURED_SHADER_PATH, nullptr, nullptr,
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

        // Input layout: POSITION(float4) + TEXCOORD(float2)
        D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
             D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        game->renderer.device->CreateInputLayout(inputElements, 2,
            vertexShaderByteCode->GetBufferPointer(),
            vertexShaderByteCode->GetBufferSize(), &layout);

        // Constant buffer (WorldViewProjData — same layout as MeshComponent)
        D3D11_BUFFER_DESC constBufDesc = {};
        constBufDesc.Usage = D3D11_USAGE_DYNAMIC;
        constBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constBufDesc.ByteWidth = sizeof(WorldViewProjData);
        game->renderer.device->CreateBuffer(&constBufDesc, nullptr, &constantBuffer);

        // Build interleaved {float4 pos, float2 uv} vertex data
        // points[] is interleaved {pos, color} pairs — even indices are positions
        struct TexVert { DirectX::XMFLOAT4 pos; DirectX::XMFLOAT2 uv; };
        vertexCount = static_cast<int>(points.size()) / 2;
        std::vector<TexVert> texVerts;
        texVerts.reserve(vertexCount);
        for (int i = 0; i < vertexCount; ++i) {
            TexVert tv;
            tv.pos = points[i * 2];
            tv.uv  = (i < static_cast<int>(meshUVs.size())) ? meshUVs[i]
                                                             : DirectX::XMFLOAT2{0.f, 0.f};
            texVerts.push_back(tv);
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(TexVert) * texVerts.size());
        D3D11_SUBRESOURCE_DATA vbData = {texVerts.data()};
        game->renderer.device->CreateBuffer(&vbDesc, &vbData, &vb);

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

        // Collider (same logic as ModelComponent::Initialize)
        collider.Radius = baseBoundingRadius * scaleVal;
        collider.Center = {worldPos.x, worldPos.y, worldPos.z};
    }

    void TexturedModelComponent::Draw() {
        game->renderer.deviceContext->RSSetState(rastState);

        D3D11_VIEWPORT viewport = {};
        viewport.Width    = static_cast<float>(game->renderer.ScreenWidth);
        viewport.Height   = static_cast<float>(game->renderer.ScreenHeight);
        viewport.MaxDepth = 1.0f;
        game->renderer.deviceContext->RSSetViewports(1, &viewport);

        game->renderer.deviceContext->IASetInputLayout(layout);
        game->renderer.deviceContext->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        constexpr UINT stride = sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2); // 24
        constexpr UINT offset = 0;
        game->renderer.deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        game->renderer.deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

        game->renderer.deviceContext->VSSetShader(vertexShader, nullptr, 0);
        game->renderer.deviceContext->PSSetShader(pixelShader, nullptr, 0);

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        game->renderer.deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &data, sizeof(WorldViewProjData));
        game->renderer.deviceContext->Unmap(constantBuffer, 0);
        game->renderer.deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

        if (srv)    game->renderer.deviceContext->PSSetShaderResources(0, 1, &srv);
        if (sampler) game->renderer.deviceContext->PSSetSamplers(0, 1, &sampler);

        game->renderer.deviceContext->DrawIndexed(
            static_cast<UINT>(indices.size()), 0, 0);
    }
}

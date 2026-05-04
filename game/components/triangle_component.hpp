//
// Created by Volkov Sergey on 26/02/2026.
//

#pragma once

#include <directxmath.h>
#include <span>
#include <vector>

#include "../game_component.hpp"
#include "consts.hpp"
#include "common_structs.h"
#include "d3d11.h"


namespace val_cg
{
    class TriangleComponent : public GameComponent //TODO: crossplatform
    {
    protected:
        ID3D11InputLayout* layout = nullptr;
        ID3D11VertexShader* vertexShader = nullptr;
        ID3DBlob* vertexShaderByteCode = nullptr;
        ID3D11RasterizerState* rastState = nullptr;
        ID3D11PixelShader* pixelShader = nullptr;
        ID3DBlob* pixelShaderByteCode = nullptr;
        ID3D11Buffer* vb = nullptr;
        ID3D11Buffer* ib = nullptr;
        std::vector<DirectX::XMFLOAT4> points;
        std::vector<int> indices;
    public:
        ///@param inputPoints vertices in format (xmfloat4)Position (xmfloat4)Color
        explicit TriangleComponent(Game* game, std::span<const DirectX::XMFLOAT4> inputPoints = {}, std::span<const int> inputIndices = {});

        void Initialize() override;
        void Draw() override;
        // void Reload() override;
        // void Update(float deltaTime) override;
        void DestroyResources() override;
    };
} // val_cg
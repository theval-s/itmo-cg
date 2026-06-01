//
// Created by Val on 8.04.2026.
//

#pragma once
#include <DirectXMath.h>
#include <SimpleMath.h>

namespace val_cg {
    struct Vertex {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT4 color;
    };
    struct WorldViewProjData {
        DirectX::SimpleMath::Matrix matrix;
    };

    struct PhongVertex {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT4 normal;
    };

  //light type
    enum LightType : int { LightDirectional = 0, LightPoint = 1 };

    struct LightData {
        DirectX::XMFLOAT4 dirOrPos;   // directional: toward-light dir; point: world position
        DirectX::XMFLOAT4 color;
        int   type;
        float attenConst;
        float attenLinear;
        float attenQuad;
    }; //48

    static constexpr int MAX_LIGHTS = 8;

    struct PhongCBData {
        DirectX::SimpleMath::Matrix worldViewProj;  // 64
        DirectX::SimpleMath::Matrix world;           // 64
        DirectX::XMFLOAT4 cameraPos;                // 16
        int   lightCount;                            // 4 +
        float _pad[3];                               // 12
        LightData lights[MAX_LIGHTS];               // 48*8 = 16*3*8
        DirectX::XMFLOAT4 matAmbient;              // 16
        DirectX::XMFLOAT4 matDiffuse;              // 16
        DirectX::XMFLOAT4 matSpecular;             // 16
    };

    static constexpr int CSM_NUM_CASCADES = 3;

    // Constant buffer for TerrainShader (slot b0).
    struct TerrainCBData {
        DirectX::SimpleMath::Matrix worldViewProj;  // 64
        DirectX::SimpleMath::Matrix world;           // 64
    }; // 128 bytes

    // Bound to slot b1 in PhongShader, updated once per frame by ShadowMapComponent.
    struct ShadowCBData {
        DirectX::SimpleMath::Matrix lightViewProj0;  // 64  – transposed for HLSL
        DirectX::SimpleMath::Matrix lightViewProj1;  // 64
        DirectX::SimpleMath::Matrix lightViewProj2;  // 64
        DirectX::XMFLOAT4 cascadeSplits;             // 16  – x,y = view-Z end of cascade 0,1
        int  shadowsEnabled;                          // 4
        float _pad[3];                               // 12
    }; // 224 bytes
}

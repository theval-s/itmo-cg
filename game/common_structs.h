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
}

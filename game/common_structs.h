//
// Created by Val on 8.04.2026.
//

#pragma once
#include "math.hpp"

namespace val_cg {
    struct Vertex {
        math::Float4 position;
        math::Float4 color;
    };
    struct WorldViewProjData {
        math::Matrix matrix;
    };

    struct PhongVertex {
        math::Float4 position;
        math::Float4 normal;
    };

  //light type
    enum LightType : int { LightDirectional = 0, LightPoint = 1, LightSpot = 2, LightAmbient = 3 };

    struct LightData {
        math::Float4 dirOrPos;   // directional: toward-light dir; point: world position
        math::Float4 color;
        int   type;
        float attenConst;
        float attenLinear;
        float attenQuad;
    }; //48

    static constexpr int MAX_LIGHTS = 8;

    struct PhongCBData {
        math::Matrix worldViewProj;  // 64
        math::Matrix world;           // 64
        math::Float4 cameraPos;                // 16
        int   lightCount;                            // 4 +
        float _pad[3];                               // 12
        LightData lights[MAX_LIGHTS];               // 48*8 = 16*3*8
        math::Float4 matAmbient;              // 16
        math::Float4 matDiffuse;              // 16
        math::Float4 matSpecular;             // 16
    };

    // ---- Deferred rendering constant buffers ----

    struct GBufferCBData {            // b0 in geometry pass
        math::Matrix worldViewProj;  // 64
        math::Matrix world;           // 64
        math::Float4 matDiffuse;               // 16
        math::Float4 matSpecular;              // 16  w = shininess
        math::Float4 objectId;                 // 16  x = per-object id (written to id RT)
    };  // 176 bytes

    struct LightingCBData {           // b0 in lighting pass
        math::Matrix invViewProj;    // 64  = (view*proj).Invert().Transpose()
        math::Matrix view;           // 64  for cascade-split view-Z
        math::Float4 cameraPos;                // 16
        math::Float4 screenSize;               // 16  xy = width, height
        math::Float4 ambient;                  // 16  used for ambient pass
        math::Float4 lightDirOrPos;            // 16  direction (dir) or position (point/spot)
        math::Float4 lightColor;               // 16
        int   lightType;                             //  4  0=dir,1=point,2=spot,3=ambient
        float attenConst;                            //  4
        float attenLinear;                           //  4
        float attenQuad;                             //  4
        math::Float4 spotDirection;            // 16  xyz = cone axis, w = cos(inner)
        float spotOuterCos;                          //  4
        float _lpad[3];                              // 12
        // CSM shadow data (directional light only)
        math::Matrix lightViewProj0; // 64
        math::Matrix lightViewProj1; // 64
        math::Matrix lightViewProj2; // 64
        math::Float4 cascadeSplits;            // 16
        int   shadowsEnabled;                        //  4
        float _spad[3];                              // 12
    };  // 464 bytes

    static constexpr int CSM_NUM_CASCADES = 3;

    // ---- GPU picking (compute shader) ----

    // b0 in the picking compute shader.
    struct PickCBData {
        math::Matrix invViewProj;    // 64  = (view*proj).Invert().Transpose()
        unsigned clickX;                             //  4  click pixel (client-space)
        unsigned clickY;                             //  4
        float    screenW;                            //  4
        float    screenH;                            //  4
    };  // 80 bytes

    // One RWStructuredBuffer element the compute shader writes; read back to CPU.
    // Tightly packed (4-byte scalars) to match the HLSL struct stride.
    struct PickResultData {
        unsigned id;                // object id under the cursor (0 = background)
        float wx, wy, wz;           // reconstructed world position
        float nx, ny, nz;           // world-space normal
    };  // 28 bytes

    // Constant buffer for TerrainShader (slot b0).
    struct TerrainCBData {
        math::Matrix worldViewProj;  // 64
        math::Matrix world;           // 64
    }; // 128 bytes

    // Bound to slot b1 in PhongShader, updated once per frame by ShadowMapComponent.
    struct ShadowCBData {
        math::Matrix lightViewProj0;  // 64  – transposed for HLSL
        math::Matrix lightViewProj1;  // 64
        math::Matrix lightViewProj2;  // 64
        math::Float4 cascadeSplits;             // 16  – x,y = view-Z end of cascade 0,1
        int  shadowsEnabled;                          // 4
        float _pad[3];                               // 12
    }; // 224 bytes
}

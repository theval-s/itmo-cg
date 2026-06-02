#pragma once
#include <d3d11.h>
#include "components/3d/gbuffer.hpp"
#include "common_structs.h"

namespace val_cg {
    class Game;
    class ShadowMapComponent;

    class RenderingSystem {
    public:
        explicit RenderingSystem(Game* game);
        void Initialize();
        void DestroyResources();

        void GeometryPass();
        void LightingPass();

        void SetShadowManager(ShadowMapComponent* sm) { shadowManager = sm; }

    private:
        void CompileShader(const wchar_t* path, const char* entry, const char* target,
                           ID3DBlob** outBlob);
        void UpdateLightingCB(const LightingCBData& data);

        Game*               game          = nullptr;
        ShadowMapComponent* shadowManager = nullptr;
        GBuffer             gbuffer;

        // --- Geometry pass resources ---
        ID3D11VertexShader* gbufferVS          = nullptr;  // POSITION+NORMAL
        ID3D11PixelShader*  gbufferPS          = nullptr;  // non-textured
        ID3D11PixelShader*  gbufferTexPS       = nullptr;  // textured (spherical UV)
        ID3D11InputLayout*  gbufferLayout      = nullptr;  // POSITION+NORMAL

        ID3D11VertexShader* gbufferTerrainVS   = nullptr;  // POSITION+COLOR(UV)
        ID3D11PixelShader*  gbufferTerrainPS   = nullptr;
        ID3D11InputLayout*  gbufferTerrainLayout = nullptr;

        ID3D11Buffer*       gbufferCB          = nullptr;  // GBufferCBData

        // --- Lighting pass resources ---
        ID3D11VertexShader* quadVS             = nullptr;  // SV_VertexID fullscreen quad
        ID3D11PixelShader*  lightingPS         = nullptr;
        ID3D11Buffer*       lightingCB         = nullptr;  // LightingCBData

        // --- Pipeline states ---
        ID3D11BlendState*        additiveBlend = nullptr;  // SrcBlend=ONE, DestBlend=ONE
        ID3D11DepthStencilState* noDepthState  = nullptr;  // no depth test or write
        ID3D11RasterizerState*   cullNoneRS    = nullptr;
    };
}

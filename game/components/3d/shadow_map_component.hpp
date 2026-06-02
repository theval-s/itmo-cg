#pragma once
#include <d3d11.h>
#include <SimpleMath.h>
#include "game_component.hpp"
#include "common_structs.h"

namespace val_cg {
    class Game;
    class DirectionalLightComponent;

    class ShadowMapComponent : public GameComponent {
    public:
        static constexpr int  NUM_CASCADES    = CSM_NUM_CASCADES;
        static constexpr int  SHADOW_MAP_SIZE = 2048;
        static constexpr float SHADOW_FAR     = 50.f;

        ShadowMapComponent(Game* game, DirectionalLightComponent* light);

        void Initialize()             override;
        void Update(float)            override {}
        void Draw()                   override {}
        void DestroyResources()       override;

        // Called by Game::Draw() before the main color pass.
        void RenderShadowMaps();

        void DrawDebugShadowMaps();

        // Bind shadow resources to PS slots (b1, t0-t2, s0).
        void BindForDraw(ID3D11DeviceContext* ctx) const;

    private:
        void ComputeCascadeSplits();
        void ComputeLightMatrices();

        DirectionalLightComponent* light;

        // Per-cascade depth targets
        ID3D11Texture2D*          shadowTextures[NUM_CASCADES] = {};
        ID3D11DepthStencilView*   shadowDSVs    [NUM_CASCADES] = {};
        ID3D11ShaderResourceView* shadowSRVs    [NUM_CASCADES] = {};

        DirectX::SimpleMath::Matrix lightVP[NUM_CASCADES];
        float cascadeSplits[NUM_CASCADES] = {};  // view-space Z end planes

        // Depth-pass shader resources
        ID3D11VertexShader*       shadowVS        = nullptr;
        ID3D11InputLayout*        shadowLayout    = nullptr;
        ID3D11Buffer*             depthPassCB     = nullptr;  // one matrix, updated per object
        ID3D11RasterizerState*    shadowRastState = nullptr;
        ID3D11DepthStencilState*  shadowDSState   = nullptr;

        // Shadow params CB sent to PhongShader (b1)
        ID3D11Buffer*             shadowParamsCB  = nullptr;
        ID3D11SamplerState*       shadowSampler   = nullptr;

        ID3D11VertexShader* debugVS = nullptr;
        ID3D11PixelShader* debugPS = nullptr;
    };
}

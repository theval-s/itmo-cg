#pragma once
#include <SimpleMath.h>
#include "game_component.hpp"
#include "common_structs.h"
#include "rhi/graphics_device.hpp"

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
        void DestroyResources()       override {}   // resources owned by GraphicsDevice

        // Called by Game::Draw() before the main color pass.
        void RenderShadowMaps();

        void DrawDebugShadowMaps();

        // Bind shadow resources to PS slots (b1, t0-t2, s0). Used by forward path.
        void BindForDraw(rhi::CommandList* cmd) const;

        // Getters for deferred lighting pass.
        const DirectX::SimpleMath::Matrix* GetLightVPs() const { return lightVP; }
        const float* GetCascadeSplits() const { return cascadeSplits; }
        rhi::GpuSampler* GetShadowSampler() const { return shadowSampler; }
        rhi::GpuTexture* GetShadowTexture(int cascade) const { return shadowTex[cascade]; }
        // Bind shadow map SRVs starting at the given PS slot (3 consecutive slots).
        void BindShadowSRVsDeferred(rhi::CommandList* cmd, int startSlot = 3) const;

    private:
        void ComputeCascadeSplits();
        void ComputeLightMatrices();

        DirectionalLightComponent* light;

        // Per-cascade depth targets (+ sampleable SRV views).
        rhi::GpuDepthTarget* shadowDepth[NUM_CASCADES] = {};
        rhi::GpuTexture*     shadowTex  [NUM_CASCADES] = {};

        DirectX::SimpleMath::Matrix lightVP[NUM_CASCADES];
        float cascadeSplits[NUM_CASCADES] = {};  // view-space Z end planes

        // Depth-pass resources
        rhi::GpuPipeline* shadowPipeline = nullptr;  // VS-only, depth-bias raster
        rhi::GpuBuffer*   depthPassCB    = nullptr;  // one matrix, updated per object
        rhi::GpuBuffer*   shadowParamsCB = nullptr;  // ShadowCBData for PhongShader (b1)
        rhi::GpuSampler*  shadowSampler  = nullptr;  // comparison sampler

        rhi::GpuPipeline* debugPipeline  = nullptr;
    };
}

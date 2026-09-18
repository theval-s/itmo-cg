#pragma once
#include "math.hpp"
#include "light_component.hpp"
#include "common_structs.h"
#include "rhi/graphics_device.hpp"

namespace val_cg {
    class MovingPointLightComponent : public LightComponent {
    public:
        MovingPointLightComponent(Game* game,
                                   math::Vector3 position,
                                   math::Vector3 direction,
                                   float speed    = 10.f,
                                   float lifetime = 4.f,
                                   math::Float3 color       = {1.f, 0.6f, 0.2f},
                                   float attenConst  = 1.f,
                                   float attenLinear = 0.22f,
                                   float attenQuad   = 0.20f);

        void Initialize() override;
        void Update(float deltaTime) override;
        void Draw() override;
        [[nodiscard]] LightData GetLightData() const override;

    private:
        // Debug viz: a line-list wire sphere rendered through the RHI (backend-agnostic).
        rhi::GpuPipeline* pipeline = nullptr;
        rhi::GpuBuffer*   vb       = nullptr;
        rhi::GpuBuffer*   ib       = nullptr;
        rhi::GpuBuffer*   cb       = nullptr;
        int               indexCount = 0;
        WorldViewProjData data{};

        math::Vector3 position;
        math::Vector3 direction;
        float speed;
        float lifetime;
        float age = 0.f;
        math::Float3 color;
        float attenConst;
        float attenLinear;
        float attenQuad;
    };
}

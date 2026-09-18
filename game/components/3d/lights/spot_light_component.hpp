#pragma once
#include "math.hpp"
#include "light_component.hpp"

namespace val_cg {
    class SpotLightComponent : public LightComponent {
    public:
        SpotLightComponent(Game* game,
                           math::Vector3 position,
                           math::Vector3 direction,
                           float innerAngleDeg = 15.f,
                           float outerAngleDeg = 30.f,
                           math::Float3 color = {1.f, 1.f, 1.f},
                           float attenConst  = 1.f,
                           float attenLinear = 0.09f,
                           float attenQuad   = 0.032f);

        [[nodiscard]] LightData GetLightData() const override;

        math::Vector3 position;
        math::Vector3 direction;  // must be normalized
        float innerAngleDeg;
        float outerAngleDeg;
        math::Float3 color;
        float attenConst;
        float attenLinear;
        float attenQuad;

        float InnerCos() const;
        float OuterCos() const;
    };
}

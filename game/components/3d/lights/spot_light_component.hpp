#pragma once
#include "light_component.hpp"
#include <SimpleMath.h>

namespace val_cg {
    class SpotLightComponent : public LightComponent {
    public:
        SpotLightComponent(Game* game,
                           DirectX::SimpleMath::Vector3 position,
                           DirectX::SimpleMath::Vector3 direction,
                           float innerAngleDeg = 15.f,
                           float outerAngleDeg = 30.f,
                           DirectX::XMFLOAT3 color = {1.f, 1.f, 1.f},
                           float attenConst  = 1.f,
                           float attenLinear = 0.09f,
                           float attenQuad   = 0.032f);

        [[nodiscard]] LightData GetLightData() const override;

        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector3 direction;  // must be normalized
        float innerAngleDeg;
        float outerAngleDeg;
        DirectX::XMFLOAT3 color;
        float attenConst;
        float attenLinear;
        float attenQuad;

        float InnerCos() const;
        float OuterCos() const;
    };
}

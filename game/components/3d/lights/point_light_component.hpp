#pragma once
#include "light_component.hpp"

namespace val_cg {
    class PointLightComponent : public LightComponent {
    public:
        PointLightComponent(Game* game,
                            DirectX::XMFLOAT3 position,
                            DirectX::XMFLOAT3 color       = {1.f, 1.f, 1.f},
                            float             attenConst   = 1.f,
                            float             attenLinear  = 0.09f,
                            float             attenQuad    = 0.032f);

        [[nodiscard]] LightData GetLightData() const override;

        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
        float attenConst;
        float attenLinear;
        float attenQuad;
    };
}

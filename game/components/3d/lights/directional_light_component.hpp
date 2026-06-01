#pragma once
#include "light_component.hpp"

namespace val_cg {
    class DirectionalLightComponent : public LightComponent {
    public:
        DirectionalLightComponent(Game* game,
                                  DirectX::XMFLOAT3 direction = {-1.f, -1.f, -1.f},
                                  DirectX::XMFLOAT3 color     = { 1.f,  1.f,  1.f});

        [[nodiscard]] LightData GetLightData() const override;

        DirectX::XMFLOAT3 direction;
        DirectX::XMFLOAT3 color;
    };
}

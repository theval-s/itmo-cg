#pragma once
#include "light_component.hpp"

namespace val_cg {
    class DirectionalLightComponent : public LightComponent {
    public:
        DirectionalLightComponent(Game* game,
                                  math::Float3 direction = {-1.f, -1.f, -1.f},
                                  math::Float3 color     = { 1.f,  1.f,  1.f});

        [[nodiscard]] LightData GetLightData() const override;

        math::Float3 direction;
        math::Float3 color;
    };
}

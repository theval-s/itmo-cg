#include "directional_light_component.hpp"
#include "SimpleMath.h"

namespace val_cg {
    DirectionalLightComponent::DirectionalLightComponent(Game* game,
                                                         DirectX::XMFLOAT3 direction,
                                                         DirectX::XMFLOAT3 color)
        : LightComponent(game), direction(direction), color(color)
    {}

    LightData DirectionalLightComponent::GetLightData() const {
        DirectX::SimpleMath::Vector3 d(direction);
        d.Normalize();
        LightData ld{};
        ld.dirOrPos = {d.x, d.y, d.z, 0.f};
        ld.color    = {color.x, color.y, color.z, 0.f};
        ld.type     = LightDirectional;
        return ld;
    }
}

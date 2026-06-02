#include "spot_light_component.hpp"
#include <cmath>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    SpotLightComponent::SpotLightComponent(Game* game,
                                           Vector3 position,
                                           Vector3 direction,
                                           float innerAngleDeg,
                                           float outerAngleDeg,
                                           DirectX::XMFLOAT3 color,
                                           float attenConst,
                                           float attenLinear,
                                           float attenQuad)
        : LightComponent(game)
        , position(position)
        , direction(direction)
        , innerAngleDeg(innerAngleDeg)
        , outerAngleDeg(outerAngleDeg)
        , color(color)
        , attenConst(attenConst)
        , attenLinear(attenLinear)
        , attenQuad(attenQuad)
    {}

    float SpotLightComponent::InnerCos() const {
        return std::cosf(innerAngleDeg * DirectX::XM_PI / 180.f);
    }

    float SpotLightComponent::OuterCos() const {
        return std::cosf(outerAngleDeg * DirectX::XM_PI / 180.f);
    }

    LightData SpotLightComponent::GetLightData() const {
        LightData d;
        d.dirOrPos   = {position.x, position.y, position.z, 1.f};
        d.color      = {color.x, color.y, color.z, 0.f};
        d.type       = LightSpot;
        d.attenConst  = attenConst;
        d.attenLinear = attenLinear;
        d.attenQuad   = attenQuad;
        return d;
    }
}

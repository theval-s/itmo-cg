#include "point_light_component.hpp"

namespace val_cg {
    PointLightComponent::PointLightComponent(Game* game,
                                             DirectX::XMFLOAT3 position,
                                             DirectX::XMFLOAT3 color,
                                             float attenConst,
                                             float attenLinear,
                                             float attenQuad)
        : LightComponent(game)
        , position(position), color(color)
        , attenConst(attenConst), attenLinear(attenLinear), attenQuad(attenQuad)
    {}

    LightData PointLightComponent::GetLightData() const {
        LightData ld{};
        ld.dirOrPos    = {position.x, position.y, position.z, 1.f};
        ld.color       = {color.x, color.y, color.z, 0.f};
        ld.type        = LightPoint;
        ld.attenConst  = attenConst;
        ld.attenLinear = attenLinear;
        ld.attenQuad   = attenQuad;
        return ld;
    }
}

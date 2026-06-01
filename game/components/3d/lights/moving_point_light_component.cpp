#include "moving_point_light_component.hpp"
#include "game.hpp"

namespace val_cg {
    MovingPointLightComponent::MovingPointLightComponent(
        Game* game,
        DirectX::SimpleMath::Vector3 position,
        DirectX::SimpleMath::Vector3 direction,
        float speed, float lifetime,
        DirectX::XMFLOAT3 color,
        float attenConst, float attenLinear, float attenQuad)
        : LightComponent(game)
        , position(position)
        , direction(direction)
        , speed(speed), lifetime(lifetime)
        , color(color)
        , attenConst(attenConst), attenLinear(attenLinear), attenQuad(attenQuad)
    {
        this->direction.Normalize();
    }

    void MovingPointLightComponent::Initialize() {
        debugSphere = DirectX::GeometricPrimitive::CreateSphere(
            game->renderer.deviceContext, 0.3f, 8);
    }

    void MovingPointLightComponent::Update(float deltaTime) {
        if (!active) return;
        position += direction * speed * deltaTime;
        age += deltaTime;
        if (age >= lifetime) active = false;
    }

    void MovingPointLightComponent::Draw() {
        if (!active || !debugSphere) return;
        auto camData = game->GetCameraData();
        auto world = DirectX::SimpleMath::Matrix::CreateTranslation(position);
        DirectX::XMVECTORF32 col = {color.x, color.y, color.z, 1.f};
        debugSphere->Draw(world, camData.viewMatrix, camData.projMatrix, col);
    }

    LightData MovingPointLightComponent::GetLightData() const {
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

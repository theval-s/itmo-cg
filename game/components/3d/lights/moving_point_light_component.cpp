#include "moving_point_light_component.hpp"
#include "game.hpp"
// DirectXTK's GeometricPrimitive is D3D11-only debug viz; it needs the raw context.
// This is the one intentional backend escape hatch outside game/rhi/d3d11/.
#include "rhi/d3d11/d3d11_device.hpp"

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
        auto* d3d = static_cast<rhi::d3d11::D3D11Device*>(game->GetDevice());
        debugSphere = DirectX::GeometricPrimitive::CreateSphere(
            d3d->RawContext(), 0.3f, 8);
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

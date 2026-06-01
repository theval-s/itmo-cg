#pragma once
#include "light_component.hpp"
#include "SimpleMath.h"
#include <GeometricPrimitive.h>
#include <memory>

namespace val_cg {
    class MovingPointLightComponent : public LightComponent {
    public:
        MovingPointLightComponent(Game* game,
                                   DirectX::SimpleMath::Vector3 position,
                                   DirectX::SimpleMath::Vector3 direction,
                                   float speed    = 10.f,
                                   float lifetime = 4.f,
                                   DirectX::XMFLOAT3 color       = {1.f, 0.6f, 0.2f},
                                   float attenConst  = 1.f,
                                   float attenLinear = 0.22f,
                                   float attenQuad   = 0.20f);

        void Initialize() override;
        void Update(float deltaTime) override;
        void Draw() override;
        [[nodiscard]] LightData GetLightData() const override;

    private:
        std::unique_ptr<DirectX::GeometricPrimitive> debugSphere;
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector3 direction;
        float speed;
        float lifetime;
        float age = 0.f;
        DirectX::XMFLOAT3 color;
        float attenConst;
        float attenLinear;
        float attenQuad;
    };
}

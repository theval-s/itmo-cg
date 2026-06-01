#include "light_shooter_component.hpp"
#include "game.hpp"
#include "InputDevice.h"
#include "components/camera_component.hpp"
#include "components/3d/lights/moving_point_light_component.hpp"

namespace val_cg {
    void LightShooterComponent::Update(float /*deltaTime*/) {
        auto* input = game->InputHandler();
        if (!input) return;

        bool clicked = input->IsKeyDown(Keys::LeftButton);
        if (clicked && !prevClickState) {
            auto* cam = game->GetCamera();
            game->AddLightDeferred(new MovingPointLightComponent(
                game, cam->GetPosition(), cam->GetForward()));
        }
        prevClickState = clicked;
    }
}

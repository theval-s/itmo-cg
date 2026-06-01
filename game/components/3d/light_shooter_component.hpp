#pragma once
#include "game_component.hpp"

namespace val_cg {
    class LightShooterComponent : public GameComponent {
    public:
        explicit LightShooterComponent(Game* game) : GameComponent(game) {}
        void Update(float deltaTime) override;
    private:
        bool prevClickState = false;
    };
}

#pragma once
#include "game_component.hpp"
#include "common_structs.h"

namespace val_cg {
    class LightComponent : public GameComponent {
    public:
        explicit LightComponent(Game* game) : GameComponent(game) {}

        bool active = true;

        [[nodiscard]] virtual LightData GetLightData() const = 0;
    };
}

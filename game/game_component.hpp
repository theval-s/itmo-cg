//
// Created by Volkov Sergey on 26/02/2026.
//

#pragma once

namespace val_cg {
    class Game;
    class GameComponent {
    protected:
        Game* game;
    public:
        virtual ~GameComponent() = default;

        explicit GameComponent(Game *game) : game(game) {}

        virtual void Initialize(){};
        virtual void Draw(){};
        virtual void Reload(){};
        virtual void Update(float deltaTime){};
        virtual void DestroyResources(){};
    };

}

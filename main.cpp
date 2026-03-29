#include <iostream>

#include "components/ball_component.hpp"
#include "game/game.hpp"
#include "game/components/paddle_component.hpp"

int main() {
    srand(time(NULL));
    val_cg::Game* game =  new val_cg::Game(L"Game", 800, 800);
    //game->Components.push_back(new val_cg::TriangleComponent(game));
    game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Left));
    game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Right));
    game->Components.push_back(new val_cg::BallComponent(game));
    game->Run();
    return 0;
}

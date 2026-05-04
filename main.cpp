#include <iostream>

#include "components/ball_component.hpp"
#include "components/3d/planet_component.hpp"
#include "game/game.hpp"
#include "game/components/paddle_component.hpp"
#include "game/components/camera_component.hpp"

void GenerateSolarSystem() {

}

int main() {
    srand(time(NULL));
    val_cg::Game* game =  new val_cg::Game(L"Game", 800, 800);
    // Lab 1
    //game->Components.push_back(new val_cg::TriangleComponent(game));

    // Lab 2
    // game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Left));
    // game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Right));
    // game->Components.push_back(new val_cg::BallComponent(game));

    // Lab 3

    val_cg::PlanetComponent* Sun = new val_cg::PlanetComponent(game, 0.f, 0.f, 0.0f, 1.f, {1.f, 0.95f, 0.f, 1.f});
    game->Components.push_back(Sun);
    game->Components.push_back(new val_cg::PlanetComponent(game, 2.f, 0.5f, 1.f, 0.5f, {1.f, 1.f, 1.f, 1.f}, Sun));
    game->Run();
    return 0;
}

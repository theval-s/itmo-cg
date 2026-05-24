#include <iostream>

#include "components/ball_component.hpp"
#include "components/3d/ball_3d_component.hpp"
#include "components/3d/paddle_3d_component.hpp"
#include "components/3d/planet_component.hpp"
#include "game/game.hpp"
#include "game/components/paddle_component.hpp"
#include <random>
#include "game/components/camera_component.hpp"

void GenerateSolarSystem() {

}

int main() {
    srand(time(NULL));
    int lab = 0;
    std::cout << "Choose lab variant: 2(mod), 3:>";
    std::cin >> lab;

    val_cg::Game* game =  new val_cg::Game(L"Game", 800, 800);
    // Lab 1
    //game->Components.push_back(new val_cg::TriangleComponent(game));

    // Lab 2
    // game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Left));
    // game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Right));
    // game->Components.push_back(new val_cg::BallComponent(game));
    if (lab == 2) {
            //game->GetCamera()->SetPosition({0.f, 0.f, 12.f});
         game->Components.push_back(new val_cg::Paddle3DComponent(game, val_cg::Paddle3DComponent::Left));
         game->Components.push_back(new val_cg::Paddle3DComponent(game, val_cg::Paddle3DComponent::Right));
         game->Components.push_back(new val_cg::Ball3DComponent(game));
         game->Run();
         return 0;
    }

    // Lab 3
    if (lab == 3) {
        val_cg::PlanetComponent* Sun = new val_cg::PlanetComponent(game, 0.f, 0.f, 1.0f, 1.f, {1.f, 0.95f, 0.f, 1.f});
        game->Components.push_back(Sun);
        game->Components.push_back(new val_cg::PlanetComponent(game, 15.f, 0.5f, 1.f, 0.5f, {1.f, 1.f, 1.f, 1.f}, Sun));

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist1(0.1f, 1.f);
        std::uniform_real_distribution<float> dist5(0.1f, 5.f);
        val_cg::PlanetComponent *tmp = nullptr;
        for (int i = 0; i < 10; i++) {
            if (i%2 == 1) {
                game->Components.push_back(new val_cg::PlanetComponent(game, 1.f, 1.f, 1000.f, dist5(gen), {dist1(gen), dist1(gen), dist1(gen), 1}, tmp));
            } else {
                tmp = new val_cg::PlanetComponent(game, i+2.f, dist1(gen) , 1000.f, dist5(gen), {dist1(gen), dist1(gen), dist1(gen), 1}, Sun);
                game->Components.push_back(tmp);
            }
        }
        game->Run();
        return 0;
    }


    return 0;
}

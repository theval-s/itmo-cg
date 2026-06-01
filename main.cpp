#include <iostream>

#include "components/ball_component.hpp"
#include "components/3d/ball_3d_component.hpp"
#include "components/3d/paddle_3d_component.hpp"
#include "components/3d/planet_component.hpp"
#include "components/3d/model_component.hpp"
#include "components/3d/textured_model_component.hpp"
#include "components/3d/katamari_player_component.hpp"
#include "components/3d/lit_model_component.hpp"
#include "components/3d/lights/directional_light_component.hpp"
#include "components/3d/light_shooter_component.hpp"
//#include "components/3d/shadow_map_component.hpp"
#include "game/game.hpp"
#include "game/components/paddle_component.hpp"
#include <random>
#include "game/components/camera_component.hpp"

void GenerateSolarSystem(val_cg::Game* game) {
    val_cg::PlanetComponent* Sun = new val_cg::PlanetComponent(game, 0.f, 0.f, 1.0f, 1.f, {1.f, 0.95f, 0.f, 1.f});
    Sun->MakeLineList({1.f, 0.95f, 0.f, 1.f});
    game->Components.push_back(Sun);
    game->Components.push_back(new val_cg::PlanetComponent(game, 15.f, 0.5f, 1.f, 0.5f, {1.f, 1.f, 1.f, 1.f}, Sun));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist1(0.1f, 1.f);
    std::uniform_real_distribution<float> dist5(0.1f, 5.f);
    val_cg::PlanetComponent *tmp = nullptr;
    for (int i = 0; i < 10; i++) {
        if (i%2 == 1) {
            auto temptemp = new val_cg::PlanetComponent(game, 1.f, 1.f, 1000.f, dist5(gen), {dist1(gen), dist1(gen), dist1(gen), 1}, tmp);
            game->Components.push_back(temptemp);
            temptemp->MakeOrbit();
        } else {
            tmp = new val_cg::PlanetComponent(game, i+2.f, dist1(gen) , 1000.f, dist5(gen), {dist1(gen), dist1(gen), dist1(gen), 1}, Sun);
            game->Components.push_back(tmp);
            tmp->MakeOrbit();
        }
    }
}

void GenerateKatamari(val_cg::Game* game) {
    using namespace DirectX::SimpleMath;

    struct ModelDef { const char* path; DirectX::XMFLOAT4 color; float scale; };
    static constexpr ModelDef defs[] = {
        {"./models/mouse.fbx", {0.8f, 0.7f, 0.6f, 1.f}, 0.004f},
        {"./models/pizza.fbx", {0.9f, 0.6f, 0.1f, 1.f}, 0.7f},
        {"./models/wolf.fbx",  {0.5f, 0.5f, 0.7f, 1.f}, 0.01f},
        {"./models/diamond.obj", {0.f, 0.82f, 1.f, 1.f}, 0.01f},
        {"./models/mushroom/mushroom.obj", {0.5f, 0.25f, 0.25f, 1.f}, 0.2f}
    };

    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-12.f, 12.f);
    std::uniform_real_distribution<float> scaleMult(0.7f, 1.6f);

    auto* terrain = new val_cg::TerrainComponent(game,
        L"./textures/heightmap.png",
        L"./textures/terrain_diffuse.png",
        30.f, 30.f, 5.f);
    game->Components.push_back(terrain);

    auto* sun = new val_cg::DirectionalLightComponent(game, {0,1.f,0}, {1.f,0.9f,0.8f});
    game->AddLight(sun);
    //game->SetShadowManager(new val_cg::ShadowMapComponent(game, sun));

    for (int i = 0; i < 25; ++i) {
        const auto& def = defs[i % 5];
        Vector3 pos{posDist(gen), 0.f, posDist(gen)};
        if (pos.Length() < 2.f) { pos.Normalize(); pos *= 2.f; }
        pos.y = terrain->GetHeightAt(pos.x, pos.z);
        float s = def.scale * scaleMult(gen);
        if (i % 5 == 0) {
            game->Components.push_back(new val_cg::TexturedModelComponent(
                game, def.path, L"./models/mouse_diffuse.png", pos, s));
        } else if (i%5 == 3) {
            game->Components.push_back(new val_cg::LitModelComponent(game, "models/diamond.obj", pos, s));
        } else {
            game->Components.push_back(new val_cg::LitModelComponent(game, def.path, pos, s));
        }
    }

    auto* player = new val_cg::KatamariPlayerComponent(game, L"./models/owl.jpg");
    player->SetTerrain(terrain);
    game->Components.push_back(player);
    game->GetCamera()->SetOrbitTarget(&player->GetPosition(), &player->GetRadius(), 8.f);

    game->Components.push_back(new val_cg::LightShooterComponent(game));
}

int main() {
    srand(time(NULL));
    val_cg::Game* game =  new val_cg::Game(L"Game", 1600, 900);
    // Lab 1
    //game->Components.push_back(new val_cg::TriangleComponent(game));

    // Lab 2
    // game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Left));
    // game->Components.push_back(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Right));
    // game->Components.push_back(new val_cg::BallComponent(game));
            //game->GetCamera()->SetPosition({0.f, 0.f, 12.f});
     // game->Components.push_back(new val_cg::Paddle3DComponent(game, val_cg::Paddle3DComponent::Left));
     // game->Components.push_back(new val_cg::Paddle3DComponent(game, val_cg::Paddle3DComponent::Right));
     // game->Components.push_back(new val_cg::Ball3DComponent(game));
     // game->Run();
     // return 0;

    // std::cout << "lab3(mod) or lab4:>";
    // std::cin >> input;

    // Lab 3
    //GenerateSolarSystem(game);


    // Lab 4-5 – Katamari
    GenerateKatamari(game);

    game->Run();
    return 0;
}

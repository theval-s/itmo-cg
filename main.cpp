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
#include "components/3d/shadow_map_component.hpp"
#include "components/3d/lights/spot_light_component.hpp"
#include "components/3d/particle_system_component.hpp"
#include "game/game.hpp"
#include "game/components/paddle_component.hpp"
#include <random>
#include <cstdlib>
#include <string>
#include "game/components/camera_component.hpp"

void GenerateSolarSystem(val_cg::Game* game) {
    val_cg::PlanetComponent* Sun = new val_cg::PlanetComponent(game, 0.f, 0.f, 1.0f, 1.f, {1.f, 0.95f, 0.f, 1.f});
    Sun->MakeLineList({1.f, 0.95f, 0.f, 1.f});
    game->AddComponent(Sun);
    game->AddComponent(new val_cg::PlanetComponent(game, 15.f, 0.5f, 1.f, 0.5f, {1.f, 1.f, 1.f, 1.f}, Sun));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist1(0.1f, 1.f);
    std::uniform_real_distribution<float> dist5(0.1f, 5.f);
    val_cg::PlanetComponent *tmp = nullptr;
    for (int i = 0; i < 10; i++) {
        if (i%2 == 1) {
            auto temptemp = new val_cg::PlanetComponent(game, 1.f, 1.f, 1000.f, dist5(gen), {dist1(gen), dist1(gen), dist1(gen), 1}, tmp);
            game->AddComponent(temptemp);
            temptemp->MakeOrbit();
        } else {
            tmp = new val_cg::PlanetComponent(game, i+2.f, dist1(gen) , 1000.f, dist5(gen), {dist1(gen), dist1(gen), dist1(gen), 1}, Sun);
            game->AddComponent(tmp);
            tmp->MakeOrbit();
        }
    }
}

// Katamari scene, grown lab by lab
//   4  terrain + coloured/textured models + player       (forward, unlit)
//   5  + directional light                               (forward Phong)
//   6  + cascaded shadow maps                            (forward CSM)
//   7  + spot light, light shooter                       (deferred G-buffer + lighting)
//   8  + GPU particle fountain                           (deferred + compute)
void GenerateKatamari(val_cg::Game* game, int lab) {
    using namespace val_cg::math;

    struct ModelDef { const char* path; Float4 color; float scale; };
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
        "./textures/heightmap.png",
        "./textures/terrain_diffuse.png",
        30.f, 30.f, 5.f);
    game->AddComponent(terrain);

    if (lab >= 5) {
        auto* sun = new val_cg::DirectionalLightComponent(game, {1,1.f,0}, {1.f,0.9f,0.8f});
        game->AddLight(sun);
        if (lab >= 6)
            game->SetShadowManager(new val_cg::ShadowMapComponent(game, sun));
    }

    for (int i = 0; i < 25; ++i) {
        const auto& def = defs[i % 5];
        Vector3 pos{posDist(gen), 0.f, posDist(gen)};
        if (pos.Length() < 2.f) { pos.Normalize(); pos *= 2.f; }
        pos.y = terrain->GetHeightAt(pos.x, pos.z);
        float s = def.scale * scaleMult(gen);
        if (i % 5 == 0) {
            game->AddComponent(new val_cg::TexturedModelComponent(
                game, def.path, "./models/mouse_diffuse.png", pos, s));
        } else if (lab < 5) {
            game->AddComponent(new val_cg::ModelComponent(game, def.path, pos, s, def.color));
        } else if (i%5 == 3) {
            game->AddComponent(new val_cg::LitModelComponent(game, "models/diamond.obj", pos, s));
        } else {
            game->AddComponent(new val_cg::LitModelComponent(game, def.path, pos, s));
        }
    }

    auto* player = new val_cg::KatamariPlayerComponent(game);
    player->SetTerrain(terrain);
    game->AddComponent(player);
    game->GetCamera()->SetOrbitTarget(&player->GetPosition(), &player->GetRadius(), 8.f);

    if (lab >= 7) {
        game->AddComponent(new val_cg::LightShooterComponent(game));

        // Spot light pointing straight down over the starting area
        game->AddLight(new val_cg::SpotLightComponent(
            game,
            Vector3{0.f, 6.f, 0.f},   // position: above centre
            Vector3{0.f, -1.f, 0.f},  // direction: straight down
            15.f,   // inner cone half-angle (degrees)
            35.f,   // outer cone half-angle (degrees)
            {0.9f, 0.8f, 1.0f}  // cool white
        ));
    }

    if (lab >= 8) {
        // Particle fountain at the centre of the scene.
        Vector3 fountainPos{0.f, 0.f, 0.f};
        fountainPos.y = terrain->GetHeightAt(fountainPos.x, fountainPos.z);
        game->AddComponent(new val_cg::ParticleSystemComponent(game, fountainPos));
    }
}

// Lab selection: `./itmo_cg.exe <lab>` or env ITMO_LAB=<lab>; default is 8.
//   1      triangle
//   2      pong (3D paddles/ball)      2d  pong (2D, lab 2 first half)
//   3      solar system
//   4..8   Katamari (see GenerateKatamari)
// Labs 1–6 run the forward path (no RenderingSystem); 7–8 run deferred.
std::string SelectLab(int argc, char** argv) {
    if (argc > 1 && argv[1][0] != '\0') return argv[1];
    if (const char* env = std::getenv("ITMO_LAB"); env && env[0] != '\0') return env;
    return "8";
}

int main(int argc, char** argv) {
    srand(time(NULL));

    const std::string labStr = SelectLab(argc, argv);
    const bool pong3d = (labStr == "3d");
    const int  lab    = pong3d ? 2 : std::atoi(labStr.c_str());
    if (lab < 1 || lab > 8) {
        std::cerr << "Unknown lab '" << labStr << "' (expected 1..8 or 3d)\n";
        return 1;
    }
    std::cout << "Lab " << labStr << (lab >= 7 ? " (deferred)" : " (forward)") << "\n";

    val_cg::Game* game = new val_cg::Game("Game", 1600, 900, /*deferred*/ lab >= 7);

    switch (lab) {
        case 1:
            game->AddComponent(new val_cg::TriangleComponent(game));
            break;
        case 2:
            if (!pong3d) {
                game->AddComponent(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Left));
                game->AddComponent(new val_cg::PaddleComponent(game, val_cg::PaddleComponent::Right));
                game->AddComponent(new val_cg::BallComponent(game));
            } else {
                game->GetCamera()->SetPosition({0.f, 0.f, 12.f});
                game->AddComponent(new val_cg::Paddle3DComponent(game, val_cg::Paddle3DComponent::Left));
                game->AddComponent(new val_cg::Paddle3DComponent(game, val_cg::Paddle3DComponent::Right));
                game->AddComponent(new val_cg::Ball3DComponent(game));
            }
            break;
        case 3:
            GenerateSolarSystem(game);
            break;
        default:
            GenerateKatamari(game, lab);
            break;
    }

    game->Run();
    delete game;
    return 0;
}

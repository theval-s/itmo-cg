cmake_minimum_required(VERSION 3.30)
project(itmo_cg)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})


add_executable(itmo_cg main.cpp
        game/game.cpp
        game/components/triangle_component.cpp
        game/components/paddle_component.cpp
        game/components/ball_component.cpp
        game/components/3d/mesh_component.cpp
        game/platforms/display_win32.cpp
        external/InputDevice.cpp
        external/Delegates.cpp
        game/components/3d/planet_component.cpp
        game/components/camera_component.cpp
        game/components/3d/paddle_3d_component.cpp
        game/components/3d/ball_3d_component.cpp
        game/utils/geometry_generator.cpp
        game/components/3d/model_component.cpp
        game/components/3d/katamari_player_component.cpp
        game/components/3d/textured_model_component.cpp
        game/components/3d/lit_model_component.cpp
        game/components/3d/shadow_map_component.cpp
        game/components/3d/lights/directional_light_component.cpp
        game/components/3d/lights/point_light_component.cpp
        game/components/3d/lights/moving_point_light_component.cpp
        game/components/3d/light_shooter_component.cpp
        game/components/3d/lit_mesh_component.cpp
        game/components/3d/lights/spot_light_component.cpp
        game/rendering/gbuffer.cpp
        game/rendering/rendering_system.cpp
        game/rhi/d3d11/d3d11_device.cpp
        game/rhi/d3d11/d3d11_command_list.cpp
        game/rhi/d3d11/d3d11_state_cache.cpp
)

target_include_directories(itmo_cg PRIVATE external game)
target_link_libraries(itmo_cg
        d3d11
        dxgi
        d3dcompiler
)

find_package(directxtk CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
target_link_libraries(itmo_cg assimp::assimp)
target_link_libraries(itmo_cg Microsoft::DirectXTK)



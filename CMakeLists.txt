cmake_minimum_required(VERSION 3.30)
project(itmo_cg)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})


# Platform-agnostic sources: engine core, scene components, RHI interface,
# and the deferred renderer. None of these include a backend header.
add_executable(itmo_cg main.cpp
        game/game.cpp
        game/components/triangle_component.cpp
        game/components/paddle_component.cpp
        game/components/ball_component.cpp
        game/components/3d/mesh_component.cpp
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
        external/Delegates.cpp
        # Windowing + input are platform-specific
        # needs to be fixed later
        game/platforms/display_win32.cpp
        external/InputDevice.cpp
)

target_include_directories(itmo_cg PRIVATE external game)

# ---- Graphics backend selection (the RHI seam) ----
if(WIN32)
    target_sources(itmo_cg PRIVATE
            game/rhi/d3d11/d3d11_device.cpp
            game/rhi/d3d11/d3d11_command_list.cpp
            game/rhi/d3d11/d3d11_state_cache.cpp
    )
    target_link_libraries(itmo_cg d3d11 dxgi d3dcompiler)
elseif(APPLE)
    # PLACEHOLDER
    #   target_sources(itmo_cg PRIVATE game/rhi/metal/metal_device.mm
    #                                  game/rhi/metal/metal_command_list.mm ...)
    #   target_link_libraries(itmo_cg "-framework Metal" "-framework QuartzCore"
    #                                 "-framework AppKit")
    # (Also: macOS windowing/input + a cross-platform math lib to replace SimpleMath.)
    message(FATAL_ERROR "macOS/Metal not implemented yet")
else()
    message(FATAL_ERROR "No support")
endif()

# DirectXTK (SimpleMath math types + WICTextureLoader in the D3D11 backend) and
# Assimp (model loading) come from vcpkg.
find_package(directxtk CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
target_link_libraries(itmo_cg assimp::assimp)
target_link_libraries(itmo_cg Microsoft::DirectXTK)

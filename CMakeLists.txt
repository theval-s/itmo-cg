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
        game/components/3d/particle_system_component.cpp
        game/rendering/gbuffer.cpp
        game/rendering/rendering_system.cpp
        external/Delegates.cpp
        external/InputDevice.cpp          # platform-neutral key/mouse state; fed by the Display
)

target_include_directories(itmo_cg PRIVATE external game)

# ---- Platform + graphics backend selection (the RHI seam) ----
# Each platform provides a Display (game/platforms/) and a GraphicsDevice (game/rhi/<api>/).
if(WIN32)
    target_sources(itmo_cg PRIVATE
            game/platforms/display_win32.cpp
            game/rhi/d3d11/d3d11_device.cpp
            game/rhi/d3d11/d3d11_command_list.cpp
            game/rhi/d3d11/d3d11_state_cache.cpp
    )
    target_link_libraries(itmo_cg d3d11 dxgi d3dcompiler)
elseif(APPLE)
    # PLACEHOLDER — see METAL_PORT.md phase B.1
    #   target_sources(itmo_cg PRIVATE game/platforms/display_mac.mm
    #                                  game/rhi/metal/metal_device.cpp
    #                                  game/rhi/metal/metal_command_list.cpp ...)
    #   target_link_libraries(itmo_cg "-framework Metal" "-framework QuartzCore"
    #                                 "-framework AppKit")
    message(FATAL_ERROR "macOS/Metal not implemented yet")
else()
    message(FATAL_ERROR "No support")
endif()

# ---- Math ----
# The engine's math is DirectXMath + DirectXTK SimpleMath (see game/math.hpp).
# Windows: both come with the vcpkg `directxtk` package (also used for WICTextureLoader
# in the D3D11 backend). macOS: DirectXMath from the vcpkg `directxmath` port (portable,
# NEON on arm64) and SimpleMath vendored in external/SimpleMath (no DirectXTK port exists).
if(WIN32)
    find_package(directxtk CONFIG REQUIRED)
    target_link_libraries(itmo_cg Microsoft::DirectXTK)
else()
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(itmo_cg Microsoft::DirectXMath)
    target_sources(itmo_cg PRIVATE external/SimpleMath/SimpleMath.cpp)
    target_include_directories(itmo_cg PRIVATE external/SimpleMath)
endif()

# Assimp (model loading) comes from vcpkg on every platform.
find_package(assimp CONFIG REQUIRED)
target_link_libraries(itmo_cg assimp::assimp)

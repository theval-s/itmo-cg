cmake_minimum_required(VERSION 3.30)
project(itmo_cg)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})


add_executable(itmo_cg main.cpp
        game/game.cpp
        game/components/triangle_component.cpp
        game/components/paddle_component.cpp
        game/components/ball_component.cpp
        game/platforms/display_win32.cpp
        game/platforms/renderer_win32.cpp
        external/Delegates.cpp)

target_include_directories(itmo_cg PRIVATE external game)
target_link_libraries(itmo_cg
        d3d11
        dxgi
        d3dcompiler
)

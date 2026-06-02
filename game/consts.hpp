//
// Created by Val on 02.03.2026.
//

#pragma once
#include <string_view>

namespace val_cg {
    constexpr wchar_t VERTEX_SHADER_PATH[] = L"./shaders/MyVeryFirstShader.hlsl";
    constexpr wchar_t PIXEL_SHADER_PATH[] = L"./shaders/MyVeryFirstShader.hlsl";
    constexpr wchar_t PADDLE_SHADER_PATH[]=L"./shaders/PaddleShader.hlsl";
    constexpr wchar_t BALL_SHADER_PATH[]=L"./shaders/BallShader.hlsl";
    constexpr wchar_t SPHERE_SHADER_PATH[]=L"./shaders/SphereWithCamera.hlsl";
    constexpr wchar_t TEXTURED_SHADER_PATH[]=L"./shaders/TexturedShader.hlsl";
    constexpr wchar_t SIMPLE_TEXTURED_SHADER_PATH[]=L"./shaders/MegaSimpleTextureShader.hlsl";
    constexpr wchar_t PHONG_SHADER_PATH[]           =L"./shaders/PhongShader.hlsl";
    constexpr wchar_t TERRAIN_SHADER_PATH[]         =L"./shaders/TerrainShader.hlsl";
    constexpr wchar_t SHADOW_DEPTH_SHADER_PATH[]    =L"./shaders/ShadowDepth.hlsl";
    constexpr wchar_t DEBUG_SHADER_PATH[]           =L"./shaders/DebugQuad.hlsl";

    // Deferred rendering shaders
    constexpr wchar_t GBUFFER_SHADER_PATH[]          = L"./shaders/GBufferShader.hlsl";
    constexpr wchar_t GBUFFER_TEXTURED_SHADER_PATH[] = L"./shaders/GBufferTexturedShader.hlsl";
    constexpr wchar_t GBUFFER_TERRAIN_SHADER_PATH[]  = L"./shaders/GBufferTerrainShader.hlsl";
    constexpr wchar_t LIGHTING_SHADER_PATH[]         = L"./shaders/LightingShader.hlsl";
}

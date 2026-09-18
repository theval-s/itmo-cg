//
// Created by Val on 02.03.2026.
//

#pragma once
#include <string_view>

namespace val_cg {
    constexpr char VERTEX_SHADER_PATH[] = "./shaders/MyVeryFirstShader.hlsl";
    constexpr char PIXEL_SHADER_PATH[] = "./shaders/MyVeryFirstShader.hlsl";
    constexpr char PADDLE_SHADER_PATH[]= "./shaders/PaddleShader.hlsl";
    constexpr char BALL_SHADER_PATH[]= "./shaders/BallShader.hlsl";
    constexpr char SPHERE_SHADER_PATH[]= "./shaders/SphereWithCamera.hlsl";
    constexpr char TEXTURED_SHADER_PATH[]= "./shaders/TexturedShader.hlsl";
    constexpr char SIMPLE_TEXTURED_SHADER_PATH[]= "./shaders/MegaSimpleTextureShader.hlsl";
    constexpr char PHONG_SHADER_PATH[]           = "./shaders/PhongShader.hlsl";
    constexpr char TERRAIN_SHADER_PATH[]         = "./shaders/TerrainShader.hlsl";
    constexpr char SHADOW_DEPTH_SHADER_PATH[]    = "./shaders/ShadowDepth.hlsl";
    constexpr char DEBUG_SHADER_PATH[]           = "./shaders/DebugQuad.hlsl";

    // Deferred rendering shaders
    constexpr char GBUFFER_SHADER_PATH[]          = "./shaders/GBufferShader.hlsl";
    constexpr char GBUFFER_TEXTURED_SHADER_PATH[] = "./shaders/GBufferTexturedShader.hlsl";
    constexpr char GBUFFER_TERRAIN_SHADER_PATH[]  = "./shaders/GBufferTerrainShader.hlsl";
    constexpr char LIGHTING_SHADER_PATH[]         = "./shaders/LightingShader.hlsl";

    // Particle system
    constexpr char PARTICLE_SHADER_PATH[]         = "./shaders/ParticleShader.hlsl";

    // GPU picking compute shader
    constexpr char PICK_SHADER_PATH[]             = "./shaders/PickShader.hlsl";
}

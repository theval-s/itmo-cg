//
// Created by Volkov Sergey on 26/02/2026.
//

#include "game.hpp"
#include "components/3d/shadow_map_component.hpp"
#include "rendering/rendering_system.hpp"
#include <iostream>
#include <cstdio>

namespace val_cg {
    Game::Game(const char* applicationName, int clientWidth, int clientHeight, bool deferred)
        : Name(applicationName), platform(applicationName, clientWidth, clientHeight)
    {
        if (deferred) renderingSystem = new RenderingSystem(this);
        CreateCamera();
        inputDevice = new InputDevice(this);
        platform.Display().SetInputDevice(inputDevice);
    }

    Game::~Game() {
        DestroyResources();
        platform.Display().SetInputDevice(nullptr);
        delete inputDevice;
        delete camera;
        delete shadowManager;
        if (renderingSystem) { renderingSystem->DestroyResources(); delete renderingSystem; }
        // `device` (unique_ptr) frees all GPU resources after the above are gone.
    }

    void Game::DestroyResources() {
        if (shadowManager) shadowManager->DestroyResources();
        for (auto& comp : Components) {
            comp->DestroyResources();
        }
    }

    void Game::Draw() {
        auto* dev = GetDevice();
        auto* cmd = GetCommandList();

        // 1. Shadow depth pass
        if (shadowManager) shadowManager->RenderShadowMaps();

        // 2. Clear the scene depth buffer
        cmd->ClearDepth(dev->GetDepthBuffer(), 1.f);

        if (renderingSystem) {
            // 3. Deferred geometry pass: fills G-buffer + depth
            renderingSystem->GeometryPass();

            // 4. Clear backbuffer then deferred lighting pass
            const float black[4] = {0.f, 0.f, 0.f, 1.f};
            cmd->ClearRenderTarget(dev->GetBackbuffer(), black);
            renderingSystem->LightingPass();

            // 4b. GPU picking — G-buffer is filled this frame, read it before the
            // forward pass rebinds the depth buffer as a render target.
            if (pickPending) {
                renderingSystem->Pick(pickX, pickY);
                pickPending = false;
            }
        }

        // 5. Forward pass: restore targets, draw non-deferred components
        rhi::GpuRenderTarget* bb = dev->GetBackbuffer();
        cmd->SetRenderTargets(&bb, 1, dev->GetDepthBuffer());
        cmd->SetViewport(0.f, 0.f, static_cast<float>(GetWidth()), static_cast<float>(GetHeight()));
        if (!renderingSystem) {
            const float color[4] = {0.f, 0.f, 0.f, 1.f};
            cmd->ClearRenderTarget(bb, color);
        }
        for (auto& comp : Components) {
            if (renderingSystem && comp->IsDeferred()) continue;
            comp->Draw();
        }

        if (debugDraw && shadowManager) {
            shadowManager->DrawDebugShadowMaps();
        }
    }

    void Game::Exit() {
        std::cout << "Final score: " << score << std::endl;
        DestroyResources();
    }

    void Game::Run() {
        Initialize();
        PrevTime = std::chrono::steady_clock::now();
        TotalTime = 0;
        unsigned int fc = 0;

        isExitRequested = false;
        while (!isExitRequested) {
            if (!platform.Display().PumpEvents()) isExitRequested = true;

            auto curTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - PrevTime).count() / 1000.f;
            PrevTime = curTime;
            TotalTime += deltaTime;

            if (TotalTime >= 1.f) {
                float fps = fc / TotalTime;
                TotalTime -= 1.f;
                char text[256];
                std::snprintf(text, sizeof(text), "%s - FPS: %.2f", Name.c_str(), fps);
                platform.Display().SetTitle(text);
                fc = 0;
            }
            UpdateInternal();
            Update(deltaTime);
            FlushPendingLights();
            Draw();
            GetDevice()->Present();
            fc++;
        }
        Exit();
    }

    void Game::Update(float deltaTime) const {
        camera->Update(deltaTime);
        for (auto& comp : Components) {
            comp->Update(deltaTime);
        }
    }

    void Game::Initialize() {
        if (shadowManager) shadowManager->Initialize();
        if (camera) camera->Initialize();
        for (auto& comp : Components) {
            comp->Initialize();
        }
        if (renderingSystem) {
            renderingSystem->Initialize();
            renderingSystem->SetShadowManager(shadowManager);
        }
    }

    void Game::Scored() {
        score++;
        std::cout<<"Scored: "<<score<<std::endl;
    }

    void Game::CreateCamera() {
        camera = new CameraComponent(this);
        camera->SetPosition({-10.f,0.f,1.5f});
        camera->Initialize();
        isCameraCreated = true;
    }

    InputDevice* Game::InputHandler() const {
        return inputDevice;
    }

    void Game::FlushPendingLights() {
        for (auto* l : pendingLights) {
            l->objectId = nextObjectId++;
            l->Initialize();
            Components.push_back(l);
            lightSources.push_back(l);
        }
        pendingLights.clear();
    }

    void Game::UpdateInternal() {
        if (inputDevice->IsKeyDown(Keys::RightButton)) {
            debugDraw = !debugDraw;
            std::cout << "Debug Draw: " << debugDraw << std::endl;
        }

        // Edge-detect a left click and record the client-space pixel for the pick pass.
        const bool leftDown = inputDevice->IsKeyDown(Keys::LeftButton);
        if (leftDown && !prevLeftDown) {
            if (platform.Display().CursorClientPos(pickX, pickY))
                pickPending = true;
        }
        prevLeftDown = leftDown;
    }
} // val_cg

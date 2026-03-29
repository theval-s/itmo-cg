//
// Created by Volkov Sergey on 26/02/2026.
//

#include "game.hpp"

#include <iostream>

namespace val_cg {
    Game::Game(LPCWSTR applicationName, int clientWidth, int clientHeight)
        : renderer(clientWidth, clientHeight)
    {
        //inputDevice = new InputDevice(this);
    }

    void Game::DestroyResources() {
        for (auto& comp : Components) {
            comp->DestroyResources();
        }
    }

    void Game::Draw() {
        renderer.deviceContext->OMSetRenderTargets(1, &renderer.renderTargetView, nullptr);
        const float color[] = { 0.f, 0.f, 0.f, 1.0f };
        renderer.deviceContext->ClearRenderTargetView(renderer.renderTargetView, color);

        for (auto& comp : Components) {
            comp->Draw();
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
            MessageHandler();

            renderer.PrepareFrame();

            auto curTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - PrevTime).count() / 1000.f;
            //std::cerr << deltaTime << std::endl;
            PrevTime = curTime;
            TotalTime += deltaTime;

            if (TotalTime >= 1.f) {
                float fps = fc / TotalTime;
                TotalTime -= 1.f;
                WCHAR text[256];
                swprintf_s(text, L"FPS: %.2f", fps);
                SetWindowTextW(renderer.display.GetWindowHandle(), text);
                fc = 0;
            }
            Update(deltaTime);
            Draw();
            renderer.EndFrame();
            fc++;
        }
        Exit();
    }

    void Game::Update(float deltaTime) const {
        for (auto& comp : Components) {
            comp->Update(deltaTime);
        }
    }

    void Game::MessageHandler() {
        //TODO: reconsider win32 parts

        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            isExitRequested = true;
        }

        //TODO: other messages
    }

    void Game::Initialize() {
        for (auto& comp : Components) {
            comp->Initialize();
        }
    }

    void Game::Scored() {
        score++;
        std::cout<<"Scored: "<<score<<std::endl;
    }

    // InputDevice* Game::InputHandler() const {
    //     return inputDevice;
    // }

    void Game::UpdateInternal() {
        //todo???
    }
} // val_cg

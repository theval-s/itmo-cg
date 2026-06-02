//
// Created by Volkov Sergey on 26/02/2026.
//
#pragma once

#include <chrono>

#include "game_component.hpp"
#include "InputDevice.h"
#include "platforms/renderer_win32.hpp"
#include "components/camera_component.hpp"
#include "components/3d/lights/light_component.hpp"
#include "../exports.h"

namespace val_cg {
    class ShadowMapComponent;  // forward declaration

    class GAMEFRAMEWORK_API Game {
    public:
        std::chrono::steady_clock::time_point StartTime;
        std::chrono::steady_clock::time_point PrevTime;
        float TotalTime = 0;

        std::string Name;
        std::vector<GameComponent*> Components;

        RendererWin32 renderer; //display, device, swapc, devicecontext, rtv, backbuffer
    private:
        //TODO: reconsider win32 parts?

    public:
        Game(LPCWSTR applicationName=L"Game", int clientWidth=800, int clientHeight=600);
        ~Game();

        void DestroyResources();
        void Draw();
        void Exit();
        void Run();
        void Update(float deltaTime) const;
        void MessageHandler();
        void Initialize();
        void Scored();
        void CreateCamera();
        CameraData GetCameraData() const { return camera->GetCameraData();}
        CameraComponent* GetCamera() const { return camera; }
        bool IsCameraCreated() const { return isCameraCreated; }
        bool IsDebugDrawEnabled() const { return debugDraw; }

        void AddLight(LightComponent* l) {
            Components.push_back(l);
            lightSources.push_back(l);
        }
        //queue to add since those lights are added when loop is active
        void AddLightDeferred(LightComponent* l) const { pendingLights.push_back(l); }
        void FlushPendingLights();
        const std::vector<LightComponent*>& GetLights() const { return lightSources; }

        void SetShadowManager(ShadowMapComponent* sm) { shadowManager = sm; }
        ShadowMapComponent* GetShadowManager() const  { return shadowManager; }

        InputDevice* InputHandler() const;

    private:
        void UpdateInternal();


    private:
        bool isExitRequested = false;
        int score = 0;
        bool isCameraCreated = false;
        bool debugDraw = false;

        CameraComponent* camera = nullptr;
        InputDevice* inputDevice = nullptr;
        std::vector<LightComponent*> lightSources;
        mutable std::vector<LightComponent*> pendingLights;
        ShadowMapComponent* shadowManager = nullptr;
    };
} // val_cg

//
// Created by Val on 27.02.2026.
//
#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "../renderer.h"
#include "display_win32.hpp"

class RendererWin32 : Renderer {
public:
    DisplayWin32 display;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    IDXGISwapChain *swapChain = nullptr; //use ComPtr's too or not necessary?
    ID3D11DeviceContext *deviceContext = nullptr;
    ID3D11Texture2D *backBuffer = nullptr;

    ID3D11RenderTargetView *renderTargetView = nullptr;

    const int ScreenWidth;
    const int ScreenHeight;

public:
    RendererWin32(int screenWidth, int screenHeight);

    void PrepareFrame();
    void PrepareResources();
    void RestoreTargets();
    void EndFrame();

private:
    void CreateBackBuffer();

};



//
// Created by Val on 27.02.2026.
//

#include "renderer_win32.hpp"

#include <iostream>
#include <stdexcept>
#include <__msvc_ostream.hpp>

#include "d3d11.h"

RendererWin32::RendererWin32(int screenWidth, int screenHeight)
    : display("DisplayWindow", screenWidth, screenHeight), ScreenWidth(screenWidth), ScreenHeight(screenHeight) {
    D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_1};

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = screenWidth;
    swapChainDesc.BufferDesc.Height = screenHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = display.GetWindowHandle();
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SampleDesc.Count = 1; //for multisampling
    swapChainDesc.SampleDesc.Quality = 0;

    auto res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_DEBUG,
        featureLevel,
        1,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &swapChain,
        &device,
        nullptr,
        &deviceContext);

    if (FAILED(res)) {
        std::cout << "failed to create device" << std::endl;
        throw std::runtime_error("Failed to create device.");
    }
    CreateBackBuffer();

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;
    // dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    // dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    // dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    // dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

    ID3D11DepthStencilState* DSState;
    res = device->CreateDepthStencilState(&dsDesc, &DSState);
    if (FAILED(res)) {
        std::cout << "failed to create dsstate " << res << std::endl;
        throw std::runtime_error("Failed to create device.");
    }

    deviceContext->OMSetDepthStencilState(DSState, 1);

     ID3D11Texture2D* depthStencil;
     D3D11_TEXTURE2D_DESC descDepth;
     descDepth.Width = screenWidth;
     descDepth.Height = screenHeight;
     descDepth.MipLevels = 1;
     descDepth.ArraySize = 1;
     descDepth.Format = DXGI_FORMAT_D32_FLOAT;
     descDepth.SampleDesc.Count = 1;
     descDepth.SampleDesc.Quality = 0;
     descDepth.Usage = D3D11_USAGE_DEFAULT;
     descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
     descDepth.CPUAccessFlags = 0;
     descDepth.MiscFlags = 0;
     res = device->CreateTexture2D(&descDepth, nullptr, &depthStencil);
     if (FAILED(res)) {
         std::cout << "failed to create texture " << res << std::endl;
         throw std::runtime_error("Failed to create device.");
     }

     D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
     descDSV.Format = DXGI_FORMAT_D32_FLOAT;
     descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
     descDSV.Texture2D.MipSlice = 0;
     descDSV.Flags = 0;
     res = device->CreateDepthStencilView(depthStencil, &descDSV, &depthStencilView);
     if (FAILED(res)) {
         std::cout << "failed to create dsv" << std::endl;
         throw std::runtime_error("Failed to create device.");
     }
}

void RendererWin32::PrepareFrame() {
    //deviceContext->ClearState();

}

void RendererWin32::PrepareResources() {
}

void RendererWin32::RestoreTargets() {
}

void RendererWin32::EndFrame() {
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    swapChain->Present(1, 0);

}

void RendererWin32::CreateBackBuffer() {
    auto res = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(res)) {
        throw std::runtime_error("Failed to get back buffer.");
    }
    res = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    if (FAILED(res)) {
        throw std::runtime_error("Failed to create render target.");
    }
}

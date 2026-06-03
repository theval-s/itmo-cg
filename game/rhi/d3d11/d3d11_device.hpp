//
// D3D11 backend — GraphicsDevice implementation.
//
// Owns the D3D11 device/context/swapchain and every GPU resource. This is the
// migration target for everything that used to live in RendererWin32.
//
#pragma once
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <string>
#include "../graphics_device.hpp"
#include "d3d11_resources.hpp"
#include "d3d11_state_cache.hpp"
#include "d3d11_command_list.hpp"

namespace val_cg::rhi::d3d11 {

    using Microsoft::WRL::ComPtr;

    class D3D11Device : public GraphicsDevice {
    public:
        D3D11Device(HWND window, int width, int height);
        ~D3D11Device() override = default;

        GpuBuffer*   CreateBuffer(const BufferDesc& desc, const void* initialData) override;
        GpuShader*   CreateShader(const wchar_t* path, const char* entry, ShaderStage stage) override;
        GpuPipeline* CreatePipeline(const PipelineDesc& desc) override;
        GpuSampler*  CreateSampler(const SamplerDesc& desc) override;
        GpuTexture*  CreateTexture(const TextureDesc& desc) override;
        GpuTexture*  CreateTextureFromFile(const wchar_t* path) override;
        GpuRenderTarget* CreateRenderTarget(const TextureDesc& desc, GpuTexture** outTexture) override;
        GpuDepthTarget*  CreateDepthTarget(const TextureDesc& desc, GpuTexture** outTexture) override;

        GpuRenderTarget* GetBackbuffer()   override { return &backbuffer; }
        GpuDepthTarget*  GetDepthBuffer()  override { return &depthTarget; }
        GpuTexture*      GetDepthTexture() override { return &depthTexture; }
        void             Present()         override;

        CommandList* GetCommandList() override { return commandList.get(); }

        int Width()  const override { return width; }
        int Height() const override { return height; }

        // Backend escape hatch — needed while migrating subsystems that still issue
        // raw D3D11 calls. Remove once nothing outside the backend uses it.
        ID3D11Device*        RawDevice()  { return device.Get(); }
        ID3D11DeviceContext* RawContext() { return context.Get(); }

    private:
        void CreateSwapchainAndBackbuffer(HWND window);
        void CreateDepthResources();

        int width  = 0;
        int height = 0;

        ComPtr<ID3D11Device>        device;
        ComPtr<ID3D11DeviceContext> context;
        ComPtr<IDXGISwapChain>      swapChain;

        D3D11RenderTarget backbuffer;     // swapchain RTV
        D3D11DepthTarget  depthTarget;    // D32 DSV over R32_TYPELESS
        D3D11Texture      depthTexture;   // R32_FLOAT SRV of the same depth texture

        std::unique_ptr<D3D11StateCache>  stateCache;
        std::unique_ptr<D3D11CommandList> commandList;

        // Lifetime ownership of every created resource.
        std::vector<std::unique_ptr<GpuBuffer>>       buffers;
        std::vector<std::unique_ptr<D3D11Shader>>     shaders;
        std::vector<std::unique_ptr<GpuPipeline>>     pipelines;
        std::vector<std::unique_ptr<GpuTexture>>      textures;
        std::vector<std::unique_ptr<GpuRenderTarget>> renderTargets;
        std::vector<std::unique_ptr<GpuDepthTarget>>  depthTargets;
        std::vector<std::unique_ptr<GpuSampler>>      samplerObjs;

        // Shader cache key (path + entry + stage).
        struct ShaderKey { std::wstring path; std::string entry; ShaderStage stage; };
        std::vector<std::pair<ShaderKey, D3D11Shader*>> shaderCache;
    };

}

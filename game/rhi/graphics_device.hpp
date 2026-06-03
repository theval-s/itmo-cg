//
// Render Hardware Interface — the device.
//
// Owns all GPU resources and the command list. `CreateGraphicsDevice` is the
// single seam where a backend is selected (D3D11 today, Metal 4 later).
//
#pragma once
#include "rhi_enums.hpp"
#include "rhi_descs.hpp"
#include "rhi_resources.hpp"
#include "command_list.hpp"
#include <memory>

namespace val_cg::rhi {

    class GraphicsDevice {
    public:
        virtual ~GraphicsDevice() = default;

        // ---- Resource creation (device retains ownership) ----
        virtual GpuBuffer*   CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;

        // Shaders and pipelines are cached: identical requests return the same object.
        virtual GpuShader*   CreateShader(const wchar_t* path, const char* entry, ShaderStage stage) = 0;
        virtual GpuPipeline* CreatePipeline(const PipelineDesc& desc) = 0;
        virtual GpuSampler*  CreateSampler(const SamplerDesc& desc) = 0;

        virtual GpuTexture*  CreateTexture(const TextureDesc& desc) = 0;
        virtual GpuTexture*  CreateTextureFromFile(const wchar_t* path) = 0;

        // Render/depth targets. CreateRenderTarget returns the writable target;
        // pass `outTexture` to also get its sampleable SRV view.
        virtual GpuRenderTarget* CreateRenderTarget(const TextureDesc& desc, GpuTexture** outTexture = nullptr) = 0;
        virtual GpuDepthTarget*  CreateDepthTarget(const TextureDesc& desc, GpuTexture** outTexture = nullptr) = 0;

        // ---- Swapchain ----
        virtual GpuRenderTarget* GetBackbuffer()    = 0;
        virtual GpuDepthTarget*  GetDepthBuffer()   = 0;
        virtual GpuTexture*      GetDepthTexture()  = 0;  // R32_FLOAT SRV of the depth buffer
        virtual void             Present()          = 0;

        // ---- Per-frame command recording ----
        virtual CommandList* GetCommandList() = 0;

        virtual int Width()  const = 0;
        virtual int Height() const = 0;
    };

    // Backend factory. `nativeWindowHandle` is an HWND on Windows, an NSWindow* on macOS.
    std::unique_ptr<GraphicsDevice> CreateGraphicsDevice(void* nativeWindowHandle,
                                                         int width, int height);

}

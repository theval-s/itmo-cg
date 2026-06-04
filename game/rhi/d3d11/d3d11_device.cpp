//
// D3D11 backend — GraphicsDevice implementation.
//
#include "d3d11_device.hpp"
#include "d3d11_translate.hpp"
#include <d3dcompiler.h>
#include <WICTextureLoader.h>
#include <iostream>
#include <stdexcept>

namespace val_cg::rhi::d3d11 {

    D3D11Device::D3D11Device(HWND window, int width, int height)
        : width(width), height(height) {
        CreateSwapchainAndBackbuffer(window);
        CreateDepthResources();
        stateCache  = std::make_unique<D3D11StateCache>(device.Get());
        commandList = std::make_unique<D3D11CommandList>(context.Get());
    }

    void D3D11Device::CreateSwapchainAndBackbuffer(HWND window) {
        D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_1};

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount        = 2;
        sd.BufferDesc.Width   = width;
        sd.BufferDesc.Height  = height;
        sd.BufferDesc.Format  = DXGI_FORMAT_B8G8R8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator   = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = window;
        sd.Windowed     = TRUE;
        sd.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.SampleDesc.Count = 1;

        UINT createFlags = 0;
#ifndef NDEBUG
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags,
            featureLevel, 1, D3D11_SDK_VERSION, &sd, &swapChain,
            &device, nullptr, &context);
        if (FAILED(hr)) throw std::runtime_error("D3D11Device: failed to create device/swapchain");

        ComPtr<ID3D11Texture2D> back;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back.GetAddressOf()));
        if (FAILED(device->CreateRenderTargetView(back.Get(), nullptr, &backbuffer.rtv)))
            throw std::runtime_error("D3D11Device: failed to create backbuffer RTV");
    }

    void D3D11Device::CreateDepthResources() {
        D3D11_TEXTURE2D_DESC td = {};
        td.Width            = width;
        td.Height           = height;
        td.MipLevels        = 1;
        td.ArraySize        = 1;
        td.Format           = DXGI_FORMAT_R32_TYPELESS;   // viewable as D32 (DSV) and R32 (SRV)
        td.SampleDesc.Count = 1;
        td.Usage            = D3D11_USAGE_DEFAULT;
        td.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        ComPtr<ID3D11Texture2D> tex;
        if (FAILED(device->CreateTexture2D(&td, nullptr, &tex)))
            throw std::runtime_error("D3D11Device: failed to create depth texture");

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        if (FAILED(device->CreateDepthStencilView(tex.Get(), &dsvDesc, &depthTarget.dsv)))
            throw std::runtime_error("D3D11Device: failed to create depth DSV");

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format              = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        if (FAILED(device->CreateShaderResourceView(tex.Get(), &srvDesc, &depthTexture.srv)))
            throw std::runtime_error("D3D11Device: failed to create depth SRV");
        depthTexture.texture = tex;
    }

    // -------------------------------------------------------------------------
    GpuBuffer* D3D11Device::CreateBuffer(const BufferDesc& desc, const void* initialData) {
        auto buf = std::make_unique<D3D11Buffer>();
        buf->ctx     = context.Get();
        buf->dynamic = desc.dynamic;

        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(desc.byteWidth);
        bd.Usage     = desc.dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        switch (desc.type) {
            case BufferType::Vertex:   bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;   break;
            case BufferType::Index:    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;    break;
            case BufferType::Constant: bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; break;
            case BufferType::Structured:
                // SRV always (read in VS); UAV when compute writes it.
                bd.BindFlags        = D3D11_BIND_SHADER_RESOURCE | (desc.uav ? D3D11_BIND_UNORDERED_ACCESS : 0);
                bd.MiscFlags        = D3D11_RESOURCE_MISC_BUFFERSTRUCTURED;
                bd.StructureByteStride = desc.structureStride;
                break;
            case BufferType::IndirectArgs:
                bd.BindFlags = 0;
                bd.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
                break;
        }
        bd.CPUAccessFlags = desc.dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = initialData;
        device->CreateBuffer(&bd, initialData ? &init : nullptr, &buf->buffer);

        // Structured buffers get an SRV (and optionally a UAV) over their elements.
        if (desc.type == BufferType::Structured && desc.structureStride > 0) {
            const UINT numElements = static_cast<UINT>(desc.byteWidth / desc.structureStride);

            D3D11_SHADER_RESOURCE_VIEW_DESC sv = {};
            sv.Format              = DXGI_FORMAT_UNKNOWN;  // structured
            sv.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
            sv.Buffer.FirstElement = 0;
            sv.Buffer.NumElements  = numElements;
            device->CreateShaderResourceView(buf->buffer.Get(), &sv, &buf->srv);

            if (desc.uav) {
                D3D11_UNORDERED_ACCESS_VIEW_DESC uv = {};
                uv.Format              = DXGI_FORMAT_UNKNOWN;
                uv.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
                uv.Buffer.FirstElement = 0;
                uv.Buffer.NumElements  = numElements;
                uv.Buffer.Flags        = desc.append ? D3D11_BUFFER_UAV_FLAG_APPEND : 0;
                device->CreateUnorderedAccessView(buf->buffer.Get(), &uv, &buf->uav);
            }
        }

        GpuBuffer* raw = buf.get();
        buffers.push_back(std::move(buf));
        return raw;
    }

    GpuShader* D3D11Device::CreateShader(const wchar_t* path, const char* entry, ShaderStage stage) {
        // Cache hit?
        for (auto& [key, shader] : shaderCache)
            if (key.stage == stage && key.entry == entry && key.path == path)
                return shader;

        UINT flags = 0;
#ifndef NDEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        const char* target = (stage == ShaderStage::Vertex)  ? "vs_5_0"
                            : (stage == ShaderStage::Compute) ? "cs_5_0"
                                                              : "ps_5_0";

        ComPtr<ID3DBlob> blob, error;
        HRESULT hr = D3DCompileFromFile(path, nullptr, nullptr, entry, target, flags, 0,
                                        &blob, &error);
        if (FAILED(hr)) {
            if (error) std::cerr << "[D3D11Device] shader error (" << entry << "): "
                                 << static_cast<char*>(error->GetBufferPointer()) << "\n";
            throw std::runtime_error("D3D11Device: shader compilation failed");
        }

        auto shader = std::make_unique<D3D11Shader>();
        shader->stage    = stage;
        shader->bytecode = blob;
        if (stage == ShaderStage::Vertex)
            device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->vs);
        else if (stage == ShaderStage::Compute)
            device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->cs);
        else
            device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->ps);

        D3D11Shader* raw = shader.get();
        shaders.push_back(std::move(shader));
        shaderCache.push_back({{path, entry, stage}, raw});
        return raw;
    }

    GpuPipeline* D3D11Device::CreatePipeline(const PipelineDesc& desc) {
        auto pipe = std::make_unique<D3D11Pipeline>();
        auto* vs = static_cast<D3D11Shader*>(desc.vs);
        auto* ps = static_cast<D3D11Shader*>(desc.ps);
        pipe->vs = vs ? vs->vs.Get() : nullptr;
        pipe->ps = ps ? ps->ps.Get() : nullptr;
        pipe->topology = ToD3D(desc.topology);

        // Input layout (built from the VS bytecode + the attribute description).
        if (vs && !desc.layout.attributes.empty()) {
            std::vector<D3D11_INPUT_ELEMENT_DESC> elems;
            elems.reserve(desc.layout.attributes.size());
            for (auto& a : desc.layout.attributes) {
                D3D11_INPUT_ELEMENT_DESC e = {};
                e.SemanticName         = a.semantic;
                e.SemanticIndex        = a.semanticIndex;
                e.Format               = ToDXGI(a.format);
                e.InputSlot            = 0;
                e.AlignedByteOffset    = (a.offset == VertexAttribute::APPEND)
                                             ? D3D11_APPEND_ALIGNED_ELEMENT : a.offset;
                e.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
                elems.push_back(e);
            }
            device->CreateInputLayout(elems.data(), static_cast<UINT>(elems.size()),
                                      vs->bytecode->GetBufferPointer(),
                                      vs->bytecode->GetBufferSize(), &pipe->layout);
        }

        pipe->rs  = stateCache->GetRasterizer(desc.raster);
        pipe->bs  = stateCache->GetBlend(desc.blend);
        pipe->dss = stateCache->GetDepthStencil(desc.depth);

        GpuPipeline* raw = pipe.get();
        pipelines.push_back(std::move(pipe));
        return raw;
    }

    GpuSampler* D3D11Device::CreateSampler(const SamplerDesc& desc) {
        auto samp = std::make_unique<D3D11Sampler>();
        samp->sampler = stateCache->GetSampler(desc);  // dedups the underlying state
        GpuSampler* raw = samp.get();
        samplerObjs.push_back(std::move(samp));
        return raw;
    }

    GpuTexture* D3D11Device::CreateTexture(const TextureDesc& desc) {
        auto tex = std::make_unique<D3D11Texture>();

        D3D11_TEXTURE2D_DESC td = {};
        td.Width            = desc.width;
        td.Height           = desc.height;
        td.MipLevels        = 1;
        td.ArraySize        = desc.arraySize;
        td.Format           = ToDXGI(desc.format);
        td.SampleDesc.Count = 1;
        td.Usage            = D3D11_USAGE_DEFAULT;
        if (desc.usage & TextureUsage::ShaderResource) td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if (desc.usage & TextureUsage::RenderTarget)   td.BindFlags |= D3D11_BIND_RENDER_TARGET;
        if (desc.usage & TextureUsage::DepthStencil)   td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        device->CreateTexture2D(&td, nullptr, &tex->texture);

        if (desc.usage & TextureUsage::ShaderResource) {
            D3D11_SHADER_RESOURCE_VIEW_DESC sv = {};
            sv.Format              = ToDXGI(desc.format);
            sv.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
            sv.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(tex->texture.Get(), &sv, &tex->srv);
        }

        GpuTexture* raw = tex.get();
        textures.push_back(std::move(tex));
        return raw;
    }

    GpuTexture* D3D11Device::CreateTextureFromFile(const wchar_t* path) {
        auto tex = std::make_unique<D3D11Texture>();
        HRESULT hr = DirectX::CreateWICTextureFromFile(device.Get(), path, nullptr, &tex->srv);
        if (FAILED(hr))
            std::wcerr << L"[D3D11Device] failed to load texture: " << path << L"\n";
        GpuTexture* raw = tex.get();
        textures.push_back(std::move(tex));
        return raw;
    }

    GpuRenderTarget* D3D11Device::CreateRenderTarget(const TextureDesc& descIn, GpuTexture** outTexture) {
        TextureDesc desc = descIn;
        desc.usage = desc.usage | TextureUsage::RenderTarget | TextureUsage::ShaderResource;
        auto* tex = static_cast<D3D11Texture*>(CreateTexture(desc));

        auto rt = std::make_unique<D3D11RenderTarget>();
        D3D11_RENDER_TARGET_VIEW_DESC rv = {};
        rv.Format        = ToDXGI(desc.format);
        rv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(tex->texture.Get(), &rv, &rt->rtv);

        if (outTexture) *outTexture = tex;
        GpuRenderTarget* raw = rt.get();
        renderTargets.push_back(std::move(rt));
        return raw;
    }

    GpuDepthTarget* D3D11Device::CreateDepthTarget(const TextureDesc& desc, GpuTexture** outTexture) {
        const bool sampleable = (desc.usage & TextureUsage::ShaderResource);

        D3D11_TEXTURE2D_DESC td = {};
        td.Width            = desc.width;
        td.Height           = desc.height;
        td.MipLevels        = 1;
        td.ArraySize        = 1;
        td.Format           = sampleable ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_D32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage            = D3D11_USAGE_DEFAULT;
        td.BindFlags        = D3D11_BIND_DEPTH_STENCIL | (sampleable ? D3D11_BIND_SHADER_RESOURCE : 0);

        ComPtr<ID3D11Texture2D> tex;
        device->CreateTexture2D(&td, nullptr, &tex);

        auto dt = std::make_unique<D3D11DepthTarget>();
        D3D11_DEPTH_STENCIL_VIEW_DESC dv = {};
        dv.Format        = DXGI_FORMAT_D32_FLOAT;
        dv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(tex.Get(), &dv, &dt->dsv);

        if (sampleable && outTexture) {
            auto sampleTex = std::make_unique<D3D11Texture>();
            sampleTex->texture = tex;
            D3D11_SHADER_RESOURCE_VIEW_DESC sv = {};
            sv.Format              = DXGI_FORMAT_R32_FLOAT;
            sv.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
            sv.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(tex.Get(), &sv, &sampleTex->srv);
            *outTexture = sampleTex.get();
            textures.push_back(std::move(sampleTex));
        }

        GpuDepthTarget* raw = dt.get();
        depthTargets.push_back(std::move(dt));
        return raw;
    }

    void D3D11Device::Present() {
        context->OMSetRenderTargets(0, nullptr, nullptr);
        swapChain->Present(1, 0);
    }

}

// The backend-selection seam. Declared in graphics_device.hpp as val_cg::rhi::
// CreateGraphicsDevice — must be defined in that namespace (not rhi::d3d11).
namespace val_cg::rhi {
    std::unique_ptr<GraphicsDevice> CreateGraphicsDevice(void* nativeWindowHandle,
                                                         int width, int height) {
        return std::make_unique<d3d11::D3D11Device>(
            static_cast<HWND>(nativeWindowHandle), width, height);
    }
}

//
// Created by Val on 25.05.2026.
//

#include "katamari_player_component.hpp"
#include "model_component.hpp"
#include "shadow_map_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include "utils/geometry_generator.hpp"
#include <wincodec.h>
#include <d3dcompiler.h>
#include <iostream>
#include "Keys.h"
#include <cmath>
#include <algorithm>
#include <WICTextureLoader.h>

namespace val_cg {
    using namespace DirectX::SimpleMath;

    // ---- TerrainComponent ----

    TerrainComponent::TerrainComponent(Game* game,
        const wchar_t* heightMapPath, const wchar_t* diffusePath,
        float width, float depth, float maxHeight)
        : MeshComponent(game), rows_(64), cols_(64),
          width_(width), depth_(depth), maxHeight_(maxHeight),
          diffusePath_(diffusePath ? diffusePath : L"")
    {
        if (heightMapPath)
            heights_ = TryLoadFromFile(heightMapPath, rows_, cols_);
        if (heights_.empty()) {
            std::wcerr << L"[Terrain] WARNING: using flat fallback — check that heightmap exists at the given path\n";
            heights_.assign(rows_ * cols_, 0.f);   // flat, obviously not the real heightmap
        } else {
            std::wcout << L"[Terrain] loaded " << cols_ << L"x" << rows_ << L" heightmap\n";
        }

        MeshData mesh = GeometryGenerator::CreateTerrain(rows_, cols_, width_, depth_, heights_, maxHeight_);
        points.clear(); indices.clear();
        for (const auto& v : mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color);   // v.color holds UV in xy
        }
        indices = mesh.indices;
        worldMatrix = Matrix::CreateTranslation(0.f, baseY, 0.f);
    }

    void TerrainComponent::Initialize() {
        // Compile terrain shader (POSITION + UV-in-COLOR-slot)
        ID3DBlob* errorCode = nullptr;
        auto compile = [&](const char* entry, const char* target, ID3DBlob** out) {
            HRESULT hr = D3DCompileFromFile(TERRAIN_SHADER_PATH, nullptr, nullptr,
                entry, target, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, out, &errorCode);
            if (FAILED(hr)) {
                if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
                else throw std::runtime_error("TerrainShader.hlsl not found");
            }
        };
        compile("VSMain", "vs_5_0", &vertexShaderByteCode);
        compile("PSMain", "ps_5_0", &pixelShaderByteCode);

        auto* dev = game->renderer.device.Get();
        dev->CreateVertexShader(vertexShaderByteCode->GetBufferPointer(),
            vertexShaderByteCode->GetBufferSize(), nullptr, &vertexShader);
        dev->CreatePixelShader(pixelShaderByteCode->GetBufferPointer(),
            pixelShaderByteCode->GetBufferSize(), nullptr, &pixelShader);

        D3D11_INPUT_ELEMENT_DESC elems[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                          D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        dev->CreateInputLayout(elems, 2,
            vertexShaderByteCode->GetBufferPointer(), vertexShaderByteCode->GetBufferSize(), &layout);

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT; vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(DirectX::XMFLOAT4) * points.size());
        D3D11_SUBRESOURCE_DATA vbData = { points.data() };
        dev->CreateBuffer(&vbDesc, &vbData, &vb);

        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT; ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(int) * indices.size());
        D3D11_SUBRESOURCE_DATA ibData = { indices.data() };
        dev->CreateBuffer(&ibDesc, &ibData, &ib);

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC; cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; cbDesc.ByteWidth = sizeof(TerrainCBData);
        dev->CreateBuffer(&cbDesc, nullptr, &constantBuffer);

        CD3D11_RASTERIZER_DESC rastDesc = {};
        rastDesc.CullMode = D3D11_CULL_NONE; rastDesc.FillMode = D3D11_FILL_SOLID;
        dev->CreateRasterizerState(&rastDesc, &rastState);

        // Diffuse texture
        if (!diffusePath_.empty()) {
            HRESULT hr = DirectX::CreateWICTextureFromFile(dev, diffusePath_.c_str(), nullptr, &srv_);
            if (FAILED(hr))
                std::wcerr << L"[TerrainComponent] failed to load diffuse: " << diffusePath_ << L"\n";
        }

        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        dev->CreateSamplerState(&sampDesc, &sampler_);
    }

    void TerrainComponent::Draw() {
        auto* ctx = game->renderer.deviceContext;
        ctx->RSSetState(rastState);

        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(game->renderer.ScreenWidth);
        vp.Height = static_cast<float>(game->renderer.ScreenHeight);
        vp.MaxDepth = 1.f;
        ctx->RSSetViewports(1, &vp);

        const UINT stride = 32, offset = 0;
        ctx->IASetInputLayout(layout);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        ctx->VSSetShader(vertexShader, nullptr, 0);
        ctx->PSSetShader(pixelShader, nullptr, 0);

        TerrainCBData tcb;
        tcb.worldViewProj = data.matrix;  // already computed as (world*view*proj).T by Update()
        tcb.world         = worldMatrix.Transpose();

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        ctx->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &tcb, sizeof(TerrainCBData));
        ctx->Unmap(constantBuffer, 0);
        ctx->VSSetConstantBuffers(0, 1, &constantBuffer);

        // Bind shadow maps at t0-t2, shadow sampler at s0 (b1 is the shadow params CB)
        if (auto* shadowMgr = game->GetShadowManager())
            shadowMgr->BindForDraw(ctx);

        // Diffuse texture at t3, regular sampler at s1 — avoids overwriting shadow slots
        ctx->PSSetShaderResources(3, 1, &srv_);
        ctx->PSSetSamplers(1, 1, &sampler_);
        ctx->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    }

    void TerrainComponent::Update(float dt) { MeshComponent::Update(dt); }

    void TerrainComponent::DrawDeferred(ID3D11DeviceContext* ctx, ID3D11Buffer* gbCB) {
        GBufferCBData cb = {};
        const auto camData = game->GetCameraData();
        cb.worldViewProj = (worldMatrix * camData.viewMatrix * camData.projMatrix).Transpose();
        cb.world         = worldMatrix.Transpose();
        cb.matDiffuse    = {1.f, 1.f, 1.f, 1.f};   // texture provides color
        cb.matSpecular   = {0.3f, 0.3f, 0.3f, 8.f};

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        ctx->Map(gbCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &cb, sizeof(GBufferCBData));
        ctx->Unmap(gbCB, 0);
        ctx->VSSetConstantBuffers(0, 1, &gbCB);
        ctx->PSSetConstantBuffers(0, 1, &gbCB);

        // Bind diffuse texture and sampler at slots expected by GBufferTerrainShader.
        ctx->PSSetShaderResources(0, 1, &srv_);
        ctx->PSSetSamplers(0, 1, &sampler_);

        const UINT stride = 32, offset = 0;
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        ctx->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    }

    float TerrainComponent::GetHeightAt(float wx, float wz) const {
        float u = std::clamp((wx + width_  * 0.5f) / width_,  0.f, 1.f);
        float v = std::clamp((wz + depth_  * 0.5f) / depth_,  0.f, 1.f);
        float fc = u * (cols_ - 1), fr = v * (rows_ - 1);
        int c0 = (int)fc, r0 = (int)fr;
        int c1 = min(c0+1, cols_-1), r1 = min(r0+1, rows_-1);
        float tc = fc-c0, tr = fr-r0;
        float h = heights_[r0*cols_+c0]*(1-tc)*(1-tr)
                + heights_[r0*cols_+c1]*tc*(1-tr)
                + heights_[r1*cols_+c0]*(1-tc)*tr
                + heights_[r1*cols_+c1]*tc*tr;
        return baseY + h * maxHeight_;
    }

    std::vector<float> TerrainComponent::TryLoadFromFile(const wchar_t* path, int& outRows, int& outCols) {
        HRESULT coHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE)
            return {};

        IWICImagingFactory* factory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory))))
            return {};

        IWICBitmapDecoder* decoder = nullptr;
        if (FAILED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                WICDecodeMetadataCacheOnDemand, &decoder))) {
            factory->Release();
            std::wcerr << L"[TerrainComponent] height map not found: " << path << L" — using procedural\n";
            return {};
        }

        IWICBitmapFrameDecode* frame = nullptr;
        decoder->GetFrame(0, &frame);

        IWICFormatConverter* conv = nullptr;
        factory->CreateFormatConverter(&conv);
        conv->Initialize(frame, GUID_WICPixelFormat8bppGray,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

        UINT w = 0, h = 0;
        conv->GetSize(&w, &h);
        outCols = static_cast<int>(w);
        outRows = static_cast<int>(h);

        std::vector<BYTE> raw(w * h);
        conv->CopyPixels(nullptr, w, static_cast<UINT>(w * h), raw.data());

        // Flip rows: image row 0 (top) → terrain far side (+Z), matching "north = top" convention
        std::vector<float> heights(w * h);
        for (int r = 0; r < outRows; ++r)
            for (int c = 0; c < outCols; ++c)
                heights[r * outCols + c] = raw[(outRows - 1 - r) * outCols + c] / 255.f;

        conv->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return heights;
    }

    std::vector<float> TerrainComponent::ProceduralHeights(int rows, int cols) {
        std::vector<float> h(rows * cols);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float u = static_cast<float>(c) / (cols-1) * DirectX::XM_2PI * 2;
                float v = static_cast<float>(r) / (rows-1) * DirectX::XM_2PI * 2;
                float val = 0.50f * sinf(u * 0.80f) * cosf(v * 0.60f)
                          + 0.25f * sinf(u * 2.10f + 1.0f) * cosf(v * 1.70f)
                          + 0.15f * sinf(u * 3.50f) * sinf(v * 3.20f + 0.5f);
                h[r*cols+c] = std::clamp((val + 0.9f) / 1.8f, 0.f, 1.f);
            }
        }
        return h;
    }

    KatamariPlayerComponent::KatamariPlayerComponent(Game* game)
        : MeshComponent(game)
        , LitMeshComponent(game)
    {
        points.clear();
        indices.clear();
        MeshData mesh = GeometryGenerator::CreateSphere(1.0f, 16, 16, {});
        for (const auto& v : mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color); // placeholder; overwritten in Initialize()
        }
        indices = mesh.indices;
    }

    void KatamariPlayerComponent::Initialize() {
        ID3DBlob* errorCode = nullptr;
        auto* dev = game->renderer.device.Get();

        HRESULT res = D3DCompileFromFile(SIMPLE_TEXTURED_SHADER_PATH, nullptr, nullptr,
            "VSMain", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
            &vertexShaderByteCode, &errorCode);
        if (FAILED(res)) {
            if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
            else throw std::runtime_error("MegaSimpleTextureShader.hlsl not found");
        }

        res = D3DCompileFromFile(SIMPLE_TEXTURED_SHADER_PATH, nullptr, nullptr,
            "PSMain", "ps_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
            &pixelShaderByteCode, &errorCode);
        if (FAILED(res)) {
            if (errorCode) { std::cerr << (char*)errorCode->GetBufferPointer(); errorCode->Release(); }
            else throw std::runtime_error("MegaSimpleTextureShader.hlsl not found");
        }

        dev->CreateVertexShader(vertexShaderByteCode->GetBufferPointer(),
            vertexShaderByteCode->GetBufferSize(), nullptr, &vertexShader);
        dev->CreatePixelShader(pixelShaderByteCode->GetBufferPointer(),
            pixelShaderByteCode->GetBufferSize(), nullptr, &pixelShader);

        D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        dev->CreateInputLayout(inputElements, 2,
            vertexShaderByteCode->GetBufferPointer(),
            vertexShaderByteCode->GetBufferSize(), &layout);

        // Build PhongVertex buffer. Sphere of radius=1 → normal == position (w=0).
        int vertexCount = static_cast<int>(points.size()) / 2;
        std::vector<PhongVertex> phongVerts;
        phongVerts.reserve(vertexCount);
        for (int i = 0; i < vertexCount; ++i) {
            const auto& p = points[i * 2];
            phongVerts.push_back({p, {p.x, p.y, p.z, 0.f}});
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage     = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(PhongVertex) * phongVerts.size());
        D3D11_SUBRESOURCE_DATA vbData = {phongVerts.data()};
        dev->CreateBuffer(&vbDesc, &vbData, &vb);

        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage     = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(int) * indices.size());
        D3D11_SUBRESOURCE_DATA ibData = {indices.data()};
        dev->CreateBuffer(&ibDesc, &ibData, &ib);

        InitLitBuffers();

        CD3D11_RASTERIZER_DESC rastDesc = {};
        rastDesc.CullMode = D3D11_CULL_NONE;
        rastDesc.FillMode = D3D11_FILL_SOLID;
        dev->CreateRasterizerState(&rastDesc, &rastState);

        cbData.matAmbient  = {0.15f, 0.15f, 0.15f, 0.f};
        cbData.matDiffuse  = {0.9f, 0.9f, 0.9f, 0.f};
        cbData.matSpecular = {0.4f, 0.4f, 0.4f, 8.f};

        // Cobblestone diffuse texture
        DirectX::CreateWICTextureFromFile(dev, L"./textures/cobblestone.png",
            nullptr, &srv_);

        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.MaxLOD   = D3D11_FLOAT32_MAX;
        dev->CreateSamplerState(&sampDesc, &sampler_);

        rollRotation = Quaternion::Identity;
        rollMatrix   = Matrix::Identity;
        position.y   = (terrain_ ? terrain_->GetHeightAt(position.x, position.z) : TerrainComponent::baseY) + radius;
        collider.Center = {position.x, position.y, position.z};
        collider.Radius = radius;
    }

    void KatamariPlayerComponent::Update(float deltaTime) {
        auto* input = game->InputHandler();
        Vector3 moveDir = Vector3::Zero;
        if (input) {
            Vector3 camForward = Vector3::UnitZ;
            Vector3 camLeft   = Vector3::UnitX;
            if (game->IsCameraCreated()) {
                camForward = game->GetCamera()->GetForward();
                camForward.y = 0;
                if (camForward.LengthSquared() > 1e-6f) camForward.Normalize();
                camLeft = Vector3::Up.Cross(camForward);
                if (camLeft.LengthSquared() > 1e-6f) camLeft.Normalize();
            }

            if (input->IsKeyDown(Keys::Up))    moveDir += camForward;
            if (input->IsKeyDown(Keys::Down))  moveDir -= camForward;
            if (input->IsKeyDown(Keys::Left))  moveDir += camLeft;
            if (input->IsKeyDown(Keys::Right)) moveDir -= camLeft;

            // if (input->IsKeyDown(Keys::Up))    moveDir.z += 1.f;
            // if (input->IsKeyDown(Keys::Down))  moveDir.z -= 1.f;
            // if (input->IsKeyDown(Keys::Left))  moveDir.x += 1.f;
            // if (input->IsKeyDown(Keys::Right)) moveDir.x -= 1.f;
        }

        if (moveDir.LengthSquared() > 0.f) {
            moveDir.Normalize();
            float dist = speed * deltaTime;
            position += moveDir * dist;

            //keep rolling rolling rolling
            float rollAngle = dist / radius;

            Vector3 rollAxis = Vector3::Up.Cross(moveDir);

            if (rollAxis.LengthSquared() > 0.f)
            {
                rollAxis.Normalize();

                Quaternion deltaRot =
                    Quaternion::CreateFromAxisAngle(rollAxis, rollAngle);

                rollRotation = rollRotation* deltaRot;
                rollRotation.Normalize();

                rollMatrix = Matrix::CreateFromQuaternion(rollRotation);
            }
        }

        CheckCollision();

        // keep ball sitting on terrain surface
        if (terrain_)
            position.y = terrain_->GetHeightAt(position.x, position.z) + radius;

        collider.Center = {position.x, position.y, position.z};
        collider.Radius = radius;

        worldMatrix = Matrix::CreateScale(radius) * rollMatrix * Matrix::CreateTranslation(position);
        MeshComponent::Update(deltaTime);
    }

    void KatamariPlayerComponent::Draw() {
        auto* ctx = game->renderer.deviceContext;
        ctx->RSSetState(rastState);

        D3D11_VIEWPORT viewport = {};
        viewport.Width    = static_cast<float>(game->renderer.ScreenWidth);
        viewport.Height   = static_cast<float>(game->renderer.ScreenHeight);
        viewport.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &viewport);

        constexpr UINT stride = sizeof(PhongVertex), offset = 0;
        ctx->IASetInputLayout(layout);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        ctx->VSSetShader(vertexShader, nullptr, 0);
        ctx->PSSetShader(pixelShader, nullptr, 0);

        BindPhongCB(worldMatrix);
        BindShadow();

        ctx->PSSetShaderResources(3, 1, &srv_);
        ctx->PSSetSamplers(1, 1, &sampler_);

        ctx->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    }

    void KatamariPlayerComponent::DrawDeferred(ID3D11DeviceContext* ctx, ID3D11Buffer* gbCB) {
        // Upload GBufferCBData (transform + materials).
        const auto camData = game->GetCameraData();
        GBufferCBData cb = {};
        cb.worldViewProj = (worldMatrix * camData.viewMatrix * camData.projMatrix).Transpose();
        cb.world         = worldMatrix.Transpose();
        cb.matDiffuse    = cbData.matDiffuse;
        cb.matSpecular   = cbData.matSpecular;

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        ctx->Map(gbCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &cb, sizeof(GBufferCBData));
        ctx->Unmap(gbCB, 0);
        ctx->VSSetConstantBuffers(0, 1, &gbCB);
        ctx->PSSetConstantBuffers(0, 1, &gbCB);

        // Bind cobblestone texture at t0 (GBufferTexturedShader reads t0/s0).
        ctx->PSSetShaderResources(0, 1, &srv_);
        ctx->PSSetSamplers(0, 1, &sampler_);

        constexpr UINT stride = sizeof(PhongVertex), offset = 0;
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        ctx->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    }

    void KatamariPlayerComponent::CheckCollision() {
        for (auto* comp : game->Components) {
            auto* model = dynamic_cast<ModelComponent*>(comp);
            if (!model || model->IsAttached()) continue;

            if (collider.Intersects(model->GetCollider())) {
                if (model->GetObjectRadius() <= radius * 1.5f) {
                    const auto& c = model->GetCollider().Center;
                    Vector3 worldOffset = Vector3(c.x, c.y, c.z) - position;
                    model->AttachTo(&position, &rollMatrix, worldOffset);

                    radius *=1.1f;
                // position.y corrected by terrain in Update
                }
            }
        }
    }
}

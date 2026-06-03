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
#include <iostream>
#include "Keys.h"
#include <cmath>
#include <algorithm>

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
            heights_.assign(rows_ * cols_, 0.f);
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
        auto* dev = game->GetDevice();

        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(TERRAIN_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = dev->CreateShader(TERRAIN_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
        pd.layout.attributes = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"COLOR",    0, rhi::VertexFormat::Float4},
        };
        pd.raster.cull = rhi::CullMode::None;
        pipeline = dev->CreatePipeline(pd);

        vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(DirectX::XMFLOAT4) * points.size()}, points.data());
        ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},            indices.data());
        constantBuffer = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(TerrainCBData), /*dynamic*/true});

        if (!diffusePath_.empty())
            srv_ = dev->CreateTextureFromFile(diffusePath_.c_str());
        sampler_ = dev->CreateSampler({rhi::Filter::Linear, rhi::AddressMode::Wrap});
    }

    void TerrainComponent::Draw() {
        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(pipeline);
        cmd->SetVertexBuffer(vb, 32);
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);

        TerrainCBData tcb;
        tcb.worldViewProj = data.matrix;  // (world*view*proj).T from Update()
        tcb.world         = worldMatrix.Transpose();
        constantBuffer->Update(&tcb, sizeof(TerrainCBData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, constantBuffer);

        // Shadow maps at t0-t2 + sampler s0, shadow params at b1.
        if (auto* shadowMgr = game->GetShadowManager())
            shadowMgr->BindForDraw(cmd);

        // Diffuse at t3, regular sampler at s1 — avoids overwriting shadow slots.
        cmd->SetTexture(rhi::ShaderStage::Pixel, 3, srv_);
        cmd->SetSampler(rhi::ShaderStage::Pixel, 1, sampler_);
        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }

    void TerrainComponent::Update(float dt) { MeshComponent::Update(dt); }

    void TerrainComponent::DrawDeferred(rhi::CommandList* cmd, rhi::GpuBuffer* gbCB) {
        GBufferCBData cb = {};
        const auto camData = game->GetCameraData();
        cb.worldViewProj = (worldMatrix * camData.viewMatrix * camData.projMatrix).Transpose();
        cb.world         = worldMatrix.Transpose();
        cb.matDiffuse    = {1.f, 1.f, 1.f, 1.f};   // texture provides color
        cb.matSpecular   = {0.3f, 0.3f, 0.3f, 8.f};

        gbCB->Update(&cb, sizeof(GBufferCBData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, gbCB);
        cmd->SetConstantBuffer(rhi::ShaderStage::Pixel,  0, gbCB);

        // Diffuse texture + sampler at the slots GBufferTerrainShader expects (t0/s0).
        cmd->SetTexture(rhi::ShaderStage::Pixel, 0, srv_);
        cmd->SetSampler(rhi::ShaderStage::Pixel, 0, sampler_);

        cmd->SetVertexBuffer(vb, 32);
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);
        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
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

    // ---- KatamariPlayerComponent ----

    KatamariPlayerComponent::KatamariPlayerComponent(Game* game)
        : MeshComponent(game)
        , LitMeshComponent(game)
    {
        points.clear();
        indices.clear();
        MeshData mesh = GeometryGenerator::CreateSphere(1.0f, 16, 16, {});
        for (const auto& v : mesh.vertices) {
            points.push_back(v.position);
            points.push_back(v.color); // placeholder; overwritten below
        }
        indices = mesh.indices;
    }

    void KatamariPlayerComponent::Initialize() {
        auto* dev = game->GetDevice();

        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(SIMPLE_TEXTURED_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = dev->CreateShader(SIMPLE_TEXTURED_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
        pd.layout.attributes = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"COLOR",    0, rhi::VertexFormat::Float4},
        };
        pd.raster.cull = rhi::CullMode::None;
        pipeline = dev->CreatePipeline(pd);

        // Build PhongVertex buffer. Sphere of radius=1 → normal == position (w=0).
        int vertexCount = static_cast<int>(points.size()) / 2;
        std::vector<PhongVertex> phongVerts;
        phongVerts.reserve(vertexCount);
        for (int i = 0; i < vertexCount; ++i) {
            const auto& p = points[i * 2];
            phongVerts.push_back({p, {p.x, p.y, p.z, 0.f}});
        }

        vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(PhongVertex) * phongVerts.size()}, phongVerts.data());
        ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * indices.size()},           indices.data());

        InitLitBuffers();

        cbData.matAmbient  = {0.15f, 0.15f, 0.15f, 0.f};
        cbData.matDiffuse  = {0.9f, 0.9f, 0.9f, 0.f};
        cbData.matSpecular = {0.4f, 0.4f, 0.4f, 8.f};

        srv_     = dev->CreateTextureFromFile(L"./textures/cobblestone.png");
        sampler_ = dev->CreateSampler({rhi::Filter::Linear, rhi::AddressMode::Wrap});

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
        }

        if (moveDir.LengthSquared() > 0.f) {
            moveDir.Normalize();
            float dist = speed * deltaTime;
            position += moveDir * dist;

            float rollAngle = dist / radius;
            Vector3 rollAxis = Vector3::Up.Cross(moveDir);
            if (rollAxis.LengthSquared() > 0.f) {
                rollAxis.Normalize();
                Quaternion deltaRot = Quaternion::CreateFromAxisAngle(rollAxis, rollAngle);
                rollRotation = rollRotation* deltaRot;
                rollRotation.Normalize();
                rollMatrix = Matrix::CreateFromQuaternion(rollRotation);
            }
        }

        CheckCollision();

        if (terrain_)
            position.y = terrain_->GetHeightAt(position.x, position.z) + radius;

        collider.Center = {position.x, position.y, position.z};
        collider.Radius = radius;

        worldMatrix = Matrix::CreateScale(radius) * rollMatrix * Matrix::CreateTranslation(position);
        MeshComponent::Update(deltaTime);
    }

    void KatamariPlayerComponent::Draw() {
        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(pipeline);
        cmd->SetVertexBuffer(vb, sizeof(PhongVertex));
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);

        BindPhongCB(worldMatrix);
        BindShadow();

        cmd->SetTexture(rhi::ShaderStage::Pixel, 3, srv_);
        cmd->SetSampler(rhi::ShaderStage::Pixel, 1, sampler_);

        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
    }

    void KatamariPlayerComponent::DrawDeferred(rhi::CommandList* cmd, rhi::GpuBuffer* gbCB) {
        const auto camData = game->GetCameraData();
        GBufferCBData cb = {};
        cb.worldViewProj = (worldMatrix * camData.viewMatrix * camData.projMatrix).Transpose();
        cb.world         = worldMatrix.Transpose();
        cb.matDiffuse    = cbData.matDiffuse;
        cb.matSpecular   = cbData.matSpecular;

        gbCB->Update(&cb, sizeof(GBufferCBData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, gbCB);
        cmd->SetConstantBuffer(rhi::ShaderStage::Pixel,  0, gbCB);

        // Cobblestone texture at t0 (GBufferTexturedShader reads t0/s0).
        cmd->SetTexture(rhi::ShaderStage::Pixel, 0, srv_);
        cmd->SetSampler(rhi::ShaderStage::Pixel, 0, sampler_);

        cmd->SetVertexBuffer(vb, sizeof(PhongVertex));
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);
        cmd->DrawIndexed(static_cast<unsigned>(indices.size()));
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
                }
            }
        }
    }
}

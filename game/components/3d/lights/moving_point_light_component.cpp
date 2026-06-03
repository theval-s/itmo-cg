#include "moving_point_light_component.hpp"
#include "game.hpp"
#include "consts.hpp"
#include "utils/geometry_generator.hpp"

namespace val_cg {
    using namespace DirectX::SimpleMath;

    MovingPointLightComponent::MovingPointLightComponent(
        Game* game,
        DirectX::SimpleMath::Vector3 position,
        DirectX::SimpleMath::Vector3 direction,
        float speed, float lifetime,
        DirectX::XMFLOAT3 color,
        float attenConst, float attenLinear, float attenQuad)
        : LightComponent(game)
        , position(position)
        , direction(direction)
        , speed(speed), lifetime(lifetime)
        , color(color)
        , attenConst(attenConst), attenLinear(attenLinear), attenQuad(attenQuad)
    {
        this->direction.Normalize();
    }

    void MovingPointLightComponent::Initialize() {
        auto* dev = game->GetDevice();

        // Wire-sphere debug mesh in the light's colour.
        MeshData mesh = GeometryGenerator::CreateSphereLineList(
            0.3f, 8, 8, {color.x, color.y, color.z, 1.f});
        std::vector<DirectX::XMFLOAT4> pts;
        pts.reserve(mesh.vertices.size() * 2);
        for (const auto& v : mesh.vertices) {
            pts.push_back(v.position);
            pts.push_back(v.color);
        }
        indexCount = static_cast<int>(mesh.indices.size());

        rhi::PipelineDesc pd;
        pd.vs = dev->CreateShader(SPHERE_SHADER_PATH, "VSMain", rhi::ShaderStage::Vertex);
        pd.ps = dev->CreateShader(SPHERE_SHADER_PATH, "PSMain", rhi::ShaderStage::Pixel);
        pd.layout.attributes = {
            {"POSITION", 0, rhi::VertexFormat::Float4},
            {"COLOR",    0, rhi::VertexFormat::Float4},
        };
        pd.raster.cull = rhi::CullMode::None;
        pd.topology    = rhi::PrimitiveTopology::LineList;
        pipeline = dev->CreatePipeline(pd);

        vb = dev->CreateBuffer({rhi::BufferType::Vertex, sizeof(DirectX::XMFLOAT4) * pts.size()}, pts.data());
        ib = dev->CreateBuffer({rhi::BufferType::Index,  sizeof(int) * mesh.indices.size()},     mesh.indices.data());
        cb = dev->CreateBuffer({rhi::BufferType::Constant, sizeof(WorldViewProjData), /*dynamic*/true});
    }

    void MovingPointLightComponent::Update(float deltaTime) {
        if (!active) return;
        position += direction * speed * deltaTime;
        age += deltaTime;
        if (age >= lifetime) active = false;
    }

    void MovingPointLightComponent::Draw() {
        if (!active) return;
        const auto camData = game->GetCameraData();
        Matrix world = Matrix::CreateTranslation(position);
        data.matrix = (world * camData.viewMatrix * camData.projMatrix).Transpose();

        auto* cmd = game->GetCommandList();
        cmd->SetPipeline(pipeline);
        cmd->SetVertexBuffer(vb, 32);
        cmd->SetIndexBuffer(ib, rhi::IndexFormat::Uint32);
        cb->Update(&data, sizeof(WorldViewProjData));
        cmd->SetConstantBuffer(rhi::ShaderStage::Vertex, 0, cb);
        cmd->DrawIndexed(static_cast<unsigned>(indexCount));
    }

    LightData MovingPointLightComponent::GetLightData() const {
        LightData ld{};
        ld.dirOrPos    = {position.x, position.y, position.z, 1.f};
        ld.color       = {color.x, color.y, color.z, 0.f};
        ld.type        = LightPoint;
        ld.attenConst  = attenConst;
        ld.attenLinear = attenLinear;
        ld.attenQuad   = attenQuad;
        return ld;
    }
}

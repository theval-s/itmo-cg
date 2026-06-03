#pragma once
#include "mesh_component.hpp"
#include "common_structs.h"
#include "rhi/graphics_device.hpp"

namespace val_cg {
    // Base for any opaque mesh that participates in Phong lighting and CSM shadows.
    // Uses virtual inheritance so subclasses can combine it with ModelComponent
    // (both share MeshComponent) without a diamond ambiguity.
class LitMeshComponent : virtual public MeshComponent {
    public:
        explicit LitMeshComponent(Game* game);

        // Called by ShadowMapComponent during the depth pass.
        // Assumes the shadow pipeline and per-object depth CB are already bound.
        virtual void DrawDepth(rhi::CommandList* cmd);

        // Called by RenderingSystem during the geometry pass.
        // Assumes the G-buffer pipeline is already bound.
        virtual void DrawDeferred(rhi::CommandList* cmd, rhi::GpuBuffer* gbufferCB);

        // Returns true for components that also bind a texture in DrawDeferred().
        virtual bool IsTextured() const { return false; }

        bool IsDeferred() const override { return true; }

    protected:
        // Creates phongCB and shadowParamsCB. Call once from Initialize().
        void InitLitBuffers();

        // Fills phongCB from current game state (lights, camera, world matrix) and binds to b0.
        void BindPhongCB(const DirectX::SimpleMath::Matrix& worldMat);

        // Binds shadow resources (b1, t0-t2, s0). Falls back to disabled CB if no shadow manager.
        void BindShadow();

        PhongCBData     cbData{};
        rhi::GpuBuffer* phongCB        = nullptr;
        ShadowCBData    shadowCBData{};
        rhi::GpuBuffer* shadowParamsCB = nullptr;
    };
}

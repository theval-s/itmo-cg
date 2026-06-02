#pragma once
#include "mesh_component.hpp"
#include "common_structs.h"

namespace val_cg {
    // Base for any opaque mesh that participates in Phong lighting and CSM shadows.
    // Uses virtual inheritance so subclasses can combine it with ModelComponent
    // (both share MeshComponent) without a diamond ambiguity.
class LitMeshComponent : virtual public MeshComponent {
    public:
        explicit LitMeshComponent(Game* game);

        // Called by ShadowMapComponent during the depth pass.
        // Assumes the shadow VS, input layout, and depth CB are already bound.
        virtual void DrawDepth();

        // Called by RenderingSystem during the geometry pass.
        // Assumes the G-buffer shader and input layout are already bound.
        virtual void DrawDeferred(ID3D11DeviceContext* ctx, ID3D11Buffer* gbufferCB);

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

        PhongCBData   cbData{};
        ID3D11Buffer* phongCB        = nullptr;
        ShadowCBData  shadowCBData{};
        ID3D11Buffer* shadowParamsCB = nullptr;
    };
}
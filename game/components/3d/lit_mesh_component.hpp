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
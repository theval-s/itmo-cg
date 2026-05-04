//
// Created by Val on 8.04.2026.
//

#pragma once
#include <DirectXMath.h>
#include <SimpleMath.h>

namespace val_cg {
    struct Vertex {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT4 color;
    };
    struct WorldViewProjData {
        DirectX::SimpleMath::Matrix matrix;
    };
}

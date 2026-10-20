//
// literally just aliases to dx SimpleMath but we have to do some shit to get macos
//

#pragma once

#ifndef _WIN32
#include <win_types_shim.h>   // external/directxtk-simplemath on macos cmake
#endif

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <SimpleMath.h>

namespace val_cg::math {
    // Value types with operators and factories: DirectX::SimpleMath.
    using Vector2    = DirectX::SimpleMath::Vector2;
    using Vector3    = DirectX::SimpleMath::Vector3;
    using Vector4    = DirectX::SimpleMath::Vector4;
    using Matrix     = DirectX::SimpleMath::Matrix;
    using Quaternion = DirectX::SimpleMath::Quaternion;
    using Plane      = DirectX::SimpleMath::Plane;
    using Ray        = DirectX::SimpleMath::Ray;
    using Color      = DirectX::SimpleMath::Color;

    // Plain-old-data layouts for vertex structs and constant buffers: DirectXMath.
    using Float2   = DirectX::XMFLOAT2;
    using Float3   = DirectX::XMFLOAT3;
    using Float4   = DirectX::XMFLOAT4;
    using Float4x4 = DirectX::XMFLOAT4X4;

    // Colliders: DirectXCollision.
    using BoundingSphere = DirectX::BoundingSphere;
    using BoundingBox    = DirectX::BoundingBox;

    constexpr float PI      = DirectX::XM_PI;
    constexpr float TWO_PI  = DirectX::XM_2PI;
    constexpr float PI_DIV2 = DirectX::XM_PIDIV2;
}

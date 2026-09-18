//
// Math layer — the single include for vectors, matrices, quaternions and colliders.
//
// The engine's math is DirectXMath + DirectXTK SimpleMath. Both are portable
// header libraries (DirectXMath picks SSE on x64 and NEON on arm64); only two
// things stop SimpleMath compiling off-Windows, and this header supplies them:
//   * a few Win32 PODs (`RECT`, `LONG`, `UINT`) that SimpleMath::Rectangle/Viewport mention
//     (external/SimpleMath/win_types_shim.h),
//   * `sal.h` — the vcpkg `directxmath` port ships a stub on non-Windows.
// Windows uses vcpkg's DirectXTK; macOS uses the vendored copy in external/SimpleMath.
//
// Everything outside this file uses the `val_cg::math` aliases. They ARE the
// DirectX types (same layout, so `common_structs.h` stays HLSL-compatible), but
// nothing else spells `DirectX::`, so swapping the implementation is a one-file job.
//
// Conventions inherited from SimpleMath (and required by the HLSL and Metal shaders):
//   row-major matrices, row vectors (v * M), right-handed, NDC z in [0,1].
//   Matrices are `.Transpose()`d before upload to a column-major constant buffer.
//
#pragma once

#ifndef _WIN32
#include "SimpleMath/win_types_shim.h"   // external/SimpleMath — vendored, macOS only
#endif

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <SimpleMath.h>

namespace val_cg::math {
    // Value types with operators and factories (SimpleMath).
    using Vector2    = DirectX::SimpleMath::Vector2;
    using Vector3    = DirectX::SimpleMath::Vector3;
    using Vector4    = DirectX::SimpleMath::Vector4;
    using Matrix     = DirectX::SimpleMath::Matrix;
    using Quaternion = DirectX::SimpleMath::Quaternion;
    using Plane      = DirectX::SimpleMath::Plane;
    using Ray        = DirectX::SimpleMath::Ray;
    using Color      = DirectX::SimpleMath::Color;

    // Plain-old-data layouts for vertex structs and constant buffers.
    using Float2   = DirectX::XMFLOAT2;
    using Float3   = DirectX::XMFLOAT3;
    using Float4   = DirectX::XMFLOAT4;
    using Float4x4 = DirectX::XMFLOAT4X4;

    // Colliders (DirectXCollision).
    using BoundingSphere = DirectX::BoundingSphere;
    using BoundingBox    = DirectX::BoundingBox;

    constexpr float PI      = DirectX::XM_PI;
    constexpr float TWO_PI  = DirectX::XM_2PI;
    constexpr float PI_DIV2 = DirectX::XM_PIDIV2;
}

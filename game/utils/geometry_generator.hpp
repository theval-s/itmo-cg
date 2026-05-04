//
// Created by Val on 13.04.2026.
//
#pragma once
#include "common_structs.h"
#include <DirectXMath.h>
#include <vector>

namespace val_cg {
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<int> indices;
    };
    class GeometryGenerator {
    public:
        static MeshData CreateSphere(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color);
    };
}

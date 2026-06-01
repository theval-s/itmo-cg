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
        static MeshData CreateSphereLineList(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color);
        static MeshData CreateBox(float hx, float hy, float hz, DirectX::XMFLOAT4 color);
        // heights: normalized [0,1] row-major array of size rows*cols; COLOR slot holds UV
        static MeshData CreateTerrain(int rows, int cols, float width, float depth,
            const std::vector<float>& heights, float maxHeight);
    };
}

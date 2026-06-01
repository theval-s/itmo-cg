//
// Created by Val on 13.04.2026.
//

#include "geometry_generator.hpp"

namespace val_cg {
    MeshData GeometryGenerator::CreateSphere(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color) {
        MeshData data;
        //data.vertices.reserve(...);

        data.vertices.push_back(Vertex({0.0f, radius, 0.0f, 1.0f}, color));

        float phiStep = DirectX::XM_PI / stackCount;
        float thetaStep = DirectX::XM_2PI / sliceCount;

        for (int i = 1; i <= stackCount - 1; i++) {
            float phi = i * phiStep;
            for (int j = 0; j <= sliceCount; j++) {
                float theta = j * thetaStep;
                //polar coordinates and shit
                float x = radius * sinf(phi) * cosf(theta);
                float y = radius * cosf(phi);
                float z = radius * sinf(phi) * sinf(theta);

                data.vertices.push_back(Vertex({x, y, z, 1.0f}, color));
            }
        }

        data.vertices.push_back(Vertex({0.0f, -radius, 0.0f, 1.0f}, color));

        //indices
        //top
        for (int i = 1; i <= sliceCount; i++) {
            data.indices.push_back(0);
            data.indices.push_back(i+1);
            data.indices.push_back(i);
        }
        //middle
        int base = 1;
        int ringCount = sliceCount + 1;
        for (int i= 0; i < stackCount - 2; i++) {
            for (int j = 0; j < sliceCount; j++) {
                data.indices.push_back(base + i * ringCount + j);
                data.indices.push_back(base + i * ringCount + j + 1);
                data.indices.push_back(base + (i+1) * ringCount + j);

                data.indices.push_back(base + (i+1) * ringCount + j);
                data.indices.push_back(base + i * ringCount + j + 1);
                data.indices.push_back(base + (i+1) * ringCount + j + 1);
            }
        }
        //down
        int downIndex = data.vertices.size() - 1;
        base = downIndex - ringCount;
        for (int i = 0; i < sliceCount; i++) {
            data.indices.push_back(downIndex);
            data.indices.push_back(base+(i+1));
            data.indices.push_back(base+i);
        }
        return data;
    }

    MeshData GeometryGenerator::CreateSphereLineList(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color) {
        MeshData data;

        data.vertices.push_back(Vertex({0.0f, radius, 0.0f, 1.0f}, color));

        float phiStep = DirectX::XM_PI / stackCount;
        float thetaStep = DirectX::XM_2PI / sliceCount;

        for (int i = 1; i <= stackCount - 1; i++) {
            float phi = i * phiStep;
            for (int j = 0; j <= sliceCount; j++) {
                float theta = j * thetaStep;
                float x = radius * sinf(phi) * cosf(theta);
                float y = radius * cosf(phi);
                float z = radius * sinf(phi) * sinf(theta);
                data.vertices.push_back(Vertex({x, y, z, 1.0f}, color));
            }
        }

        data.vertices.push_back(Vertex({0.0f, -radius, 0.0f, 1.0f}, color));

        int base = 1;
        int ringCount = sliceCount + 1;

        // top pole to first ring
        for (int j = 0; j < sliceCount; j++) {
            data.indices.push_back(0);
            data.indices.push_back(base + j);
        }

        // horizontal rings
        for (int i = 0; i < stackCount - 1; i++) {
            for (int j = 0; j < sliceCount; j++) {
                data.indices.push_back(base + i * ringCount + j);
                data.indices.push_back(base + i * ringCount + j + 1);
            }
        }

        // vertical edges between adjacent rings
        for (int i = 0; i < stackCount - 2; i++) {
            for (int j = 0; j < sliceCount; j++) {
                data.indices.push_back(base + i * ringCount + j);
                data.indices.push_back(base + (i + 1) * ringCount + j);
            }
        }

        // last ring to bottom pole
        int downIndex = data.vertices.size() - 1;
        int lastBase = downIndex - ringCount;
        for (int i = 0; i < sliceCount; i++) {
            data.indices.push_back(lastBase + i);
            data.indices.push_back(downIndex);
        }

        return data;
    }

    MeshData GeometryGenerator::CreateBox(float hx, float hy, float hz, DirectX::XMFLOAT4 color) {
        MeshData data;
        data.vertices = {
            Vertex({-hx,-hy,-hz,1}, color),
            Vertex({ hx,-hy,-hz,1}, color),
            Vertex({ hx, hy,-hz,1}, color),
            Vertex({-hx, hy,-hz,1}, color),
            Vertex({-hx,-hy, hz,1}, color),
            Vertex({ hx,-hy, hz,1}, color),
            Vertex({ hx, hy, hz,1}, color),
            Vertex({-hx, hy, hz,1}, color),
        };
        data.indices = {
            0,2,1, 0,3,2, // front
            4,5,6, 4,6,7, // back
            0,4,7, 0,7,3, // left
            1,2,6, 1,6,5, // right
            0,1,5, 0,5,4, // bottom
            3,7,6, 3,6,2, // top
        };
        return data;
    }

    MeshData GeometryGenerator::CreateTerrain(int rows, int cols, float width, float depth,
        const std::vector<float>& heights, float maxHeight)
    {
        MeshData data;
        data.vertices.reserve(rows * cols);
        float dx = width  / (cols - 1);
        float dz = depth  / (rows - 1);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float h = heights[r * cols + c];
                float x = -width * 0.5f + c * dx;
                float z = -depth * 0.5f + r * dz;
                float u = static_cast<float>(c) / (cols - 1);
                float v = 1.f - static_cast<float>(r) / (rows - 1);
                // COLOR slot repurposed as UV (xy) for the terrain shader
                data.vertices.push_back(Vertex({x, h * maxHeight, z, 1.f}, {u, v, 0.f, 1.f}));
            }
        }

        data.indices.reserve((rows - 1) * (cols - 1) * 6);
        for (int r = 0; r < rows - 1; ++r) {
            for (int c = 0; c < cols - 1; ++c) {
                int tl = r * cols + c;
                int tr = tl + 1;
                int bl = (r + 1) * cols + c;
                int br = bl + 1;
                data.indices.push_back(tl); data.indices.push_back(bl); data.indices.push_back(tr);
                data.indices.push_back(tr); data.indices.push_back(bl); data.indices.push_back(br);
            }
        }

        return data;
    }
}

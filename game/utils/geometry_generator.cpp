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

        std::vector<DirectX::XMFLOAT3> normals(rows * cols, {0.f, 0.f, 0.f});

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float h = heights[r * cols + c];
                float x = -width * 0.5f + c * dx;
                float z = -depth * 0.5f + r * dz;
                float u = static_cast<float>(c) / (cols - 1);
                float v = 1.f - static_cast<float>(r) / (rows - 1);
                data.vertices.push_back(Vertex({x, h * maxHeight, z, 1.f}, {u, v, 0.f, 1.f}));
            }
        }

        // Unnormalized cross product of two triangle edges (b-a)x(c-a), via plain
        // float math — XMVECTOR has no usable operator- on Windows (it's __m128).
        auto faceNormal = [&](int a, int b, int c) -> DirectX::XMFLOAT3 {
            const auto& pa = data.vertices[a].position;
            const auto& pb = data.vertices[b].position;
            const auto& pc = data.vertices[c].position;
            float e1x = pb.x - pa.x, e1y = pb.y - pa.y, e1z = pb.z - pa.z;
            float e2x = pc.x - pa.x, e2y = pc.y - pa.y, e2z = pc.z - pa.z;
            return { e1y * e2z - e1z * e2y,
                     e1z * e2x - e1x * e2z,
                     e1x * e2y - e1y * e2x };
        };
        auto accum = [&](int idx, const DirectX::XMFLOAT3& n) {
            normals[idx].x += n.x; normals[idx].y += n.y; normals[idx].z += n.z;
        };

        for (int r = 0; r < rows - 1; ++r) {
            for (int c = 0; c < cols - 1; ++c) {
                int tl = r * cols + c;
                int tr = tl + 1;
                int bl = (r + 1) * cols + c;
                int br = bl + 1;

                // Area-weighted (unnormalized) face normals matching the winding below.
                DirectX::XMFLOAT3 n1 = faceNormal(tl, bl, tr);
                DirectX::XMFLOAT3 n2 = faceNormal(tr, bl, br);

                accum(tl, n1);
                accum(bl, n1); accum(bl, n2);
                accum(tr, n1); accum(tr, n2);
                accum(br, n2);

                data.indices.push_back(tl); data.indices.push_back(bl); data.indices.push_back(tr);
                data.indices.push_back(tr); data.indices.push_back(bl); data.indices.push_back(br);
            }
        }

        for (auto& n : normals) {
            float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
            if (len > 1e-6f) { n.x /= len; n.y /= len; n.z /= len; }
            else             { n = {0.f, 1.f, 0.f}; }
        }

        // Pack the horizontal normal components (x, z) into the spare color.zw lanes
        // (xy hold UV). The shader reconstructs the +Y/up component — valid because
        // heightmap terrain never overhangs, so normal.y is always >= 0.
        for (int i = 0; i < rows * cols; ++i) {
            data.vertices[i].color.z = normals[i].x;
            data.vertices[i].color.w = normals[i].z;
        }

        return data;
    }
}

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
}

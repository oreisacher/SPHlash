#include "MarchingCubes.h"

#include "MarchingCubesLookup.h"

__device__ inline float3 operator+(const float3& a, const float3& b) {
    return make_float3(
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    );
}

__device__ inline float3 interp(float3 p1, float3 p2, float v1, float v2) {
    if (abs(v1 - v2) < 0.0001f)
        return p1;

    const float t = -v1 / (v2 - v1 + 1e-6f);

    return make_float3(
      p1.x + t * (p2.x - p1.x),
      p1.y + t * (p2.y - p1.y),
      p1.z + t * (p2.z - p1.z)
    );
}

__global__ void marchingCubesKernel(
    const float* scalarField,
    const float3* normals,
    const int3 resolution,
    const float3 domainMin,
    const float3 domainMax,
    int* vertexCount,
    int* triangleCount,
    Vertex* outVertices,
    unsigned int* outIndices,
    const int maxVertices,
    const int maxIndices
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int z = blockIdx.z * blockDim.z + threadIdx.z;

    if (x >= resolution.x - 1 || y >= resolution.y - 1 || z >= resolution.z - 1)
        return;

    int cx = resolution.x - 1;
    int cy = resolution.y - 1;
    int cz = resolution.z - 1;

    float3 cellSizes = make_float3(
        (domainMax.x - domainMin.x) / cx,
        (domainMax.y - domainMin.y) / cy,
        (domainMax.z - domainMin.z) / cz
    );

    // 1D scalar field offsets
    int dx = 1;
    int dy = resolution.x;
    int dz = resolution.x * resolution.y;
    int id = x + y * resolution.x + z * resolution.x * resolution.y;

    // Scalar field cube
    float cube[8];
    cube[0] = scalarField[id               ];
    cube[1] = scalarField[id + dx          ];
    cube[2] = scalarField[id + dx + dy     ];
    cube[3] = scalarField[id      + dy     ];
    cube[4] = scalarField[id           + dz];
    cube[5] = scalarField[id + dx      + dz];
    cube[6] = scalarField[id + dx + dy + dz];
    cube[7] = scalarField[id      + dy + dz];

    // Build cube index
    int cubeIndex = 0;
    if (cube[0] < 0) cubeIndex |= 1;
    if (cube[1] < 0) cubeIndex |= 2;
    if (cube[2] < 0) cubeIndex |= 4;
    if (cube[3] < 0) cubeIndex |= 8;
    if (cube[4] < 0) cubeIndex |= 16;
    if (cube[5] < 0) cubeIndex |= 32;
    if (cube[6] < 0) cubeIndex |= 64;
    if (cube[7] < 0) cubeIndex |= 128;

    // Skip empty / full cells
    if (cubeIndex == 0 || cubeIndex == 255)
        return;

    // Normals & Positions cube
    float3 normalsCube[8];
    normalsCube[0] = normals[id               ];
    normalsCube[1] = normals[id + dx          ];
    normalsCube[2] = normals[id + dx + dy     ];
    normalsCube[3] = normals[id      + dy     ];
    normalsCube[4] = normals[id           + dz];
    normalsCube[5] = normals[id + dx      + dz];
    normalsCube[6] = normals[id + dx + dy + dz];
    normalsCube[7] = normals[id      + dy + dz];

    float3 position[8];
    position[0] = make_float3((x + 0) * cellSizes.x, (y + 0) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    position[1] = make_float3((x + 1) * cellSizes.x, (y + 0) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    position[2] = make_float3((x + 1) * cellSizes.x, (y + 1) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    position[3] = make_float3((x + 0) * cellSizes.x, (y + 1) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    position[4] = make_float3((x + 0) * cellSizes.x, (y + 0) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;
    position[5] = make_float3((x + 1) * cellSizes.x, (y + 0) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;
    position[6] = make_float3((x + 1) * cellSizes.x, (y + 1) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;
    position[7] = make_float3((x + 0) * cellSizes.x, (y + 1) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;

    // Interpolate
    float3 edgeVertices[12];
    float3 edgeNormals[12];

    for (int i = 0 ; i < 12 ; i++) {
        if (edgeTable[cubeIndex] & (1 << i)) {
            int v0 = edgeToVertices[i][0];
            int v1 = edgeToVertices[i][1];

            edgeVertices[i] = interp(position[v0], position[v1], cube[v0], cube[v1]);
            edgeNormals[i] = interp(normalsCube[v0], normalsCube[v1], cube[v0], cube[v1]);
        }
    }

    // Generate triangles
    for (int i = 0 ; triangleTable[cubeIndex][i] != -1 ; i += 3) {
        int triangleIndex = atomicAdd(triangleCount, 1);

        // Skip triangle if maxIndices is reached
        if (triangleIndex >= maxIndices / 3)
            return;

        int baseVertexIndex = atomicAdd(vertexCount, 3);

        // Skip triangle if maxVertices is reached
        if (baseVertexIndex + 2 >= maxVertices)
            return;

        // Compute hard face normal
        int edgeIndex0 = triangleTable[cubeIndex][i + 0];
        int edgeIndex1 = triangleTable[cubeIndex][i + 1];
        int edgeIndex2 = triangleTable[cubeIndex][i + 2];

        float3 v0 = edgeVertices[edgeIndex0];
        float3 v1 = edgeVertices[edgeIndex1];
        float3 v2 = edgeVertices[edgeIndex2];
        // float3 edge1 = make_float3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
        // float3 edge2 = make_float3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
        //
        // float3 faceNormal = make_float3(
        //     edge1.y * edge2.z - edge1.z * edge2.y,
        //     edge1.z * edge2.x - edge1.x * edge2.z,
        //     edge1.x * edge2.y - edge1.y * edge2.x
        // );
        //
        // float length = sqrtf(faceNormal.x * faceNormal.x +
        //                     faceNormal.y * faceNormal.y +
        //                     faceNormal.z * faceNormal.z);
        // if (length > 1e-6f) {
        //     faceNormal.x /= length;
        //     faceNormal.y /= length;
        //     faceNormal.z /= length;
        // } else {
        //     faceNormal = make_float3(0.0f, 1.0f, 0.0f);
        // }

        for (int j = 0 ; j < 3 ; j++) {
            int edgeIndex = triangleTable[cubeIndex][i + j];
            outVertices[baseVertexIndex + j].position = edgeVertices[edgeIndex];
            outVertices[baseVertexIndex + j].normal = edgeNormals[edgeIndex];
            // outVertices[baseVertexIndex + j].normal = faceNormal;
        }

        outIndices[triangleIndex * 3 + 0] = baseVertexIndex + 0;
        outIndices[triangleIndex * 3 + 1] = baseVertexIndex + 1;
        outIndices[triangleIndex * 3 + 2] = baseVertexIndex + 2;
    }
}

void launchMarchingCubesKernel(
    const float* scalarField,
    const float3* normals,
    const int3 resolution,
    const float3 domainMin,
    const float3 domainMax,
    int* vertexCount,
    int* triangleCount,
    Vertex* outVertices,
    unsigned int* outIndices,
    const int maxVertices,
    const int maxIndices
) {
    int threadsPerBlock = 256;
    int cells = (resolution.x - 1) * (resolution.y - 1) * (resolution.z - 1);
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    dim3 blockSize(8, 8, 8);
    dim3 gridSize(
        (resolution.x - 1 + blockSize.x - 1) / blockSize.x,
        (resolution.y - 1 + blockSize.y - 1) / blockSize.y,
        (resolution.z - 1 + blockSize.z - 1) / blockSize.z
    );

    marchingCubesKernel<<<gridSize, blockSize>>>(
        scalarField, normals, resolution, domainMin, domainMax, vertexCount, triangleCount, outVertices, outIndices, maxVertices, maxIndices
    );
}
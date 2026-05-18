#include "MarchingCubes.h"

#include "MarchingCubesLookup.h"
#include "utils/AABB.h"
#include "utils/VoxelField.h"

__device__ inline float3 operator+(const float3& a, const float3& b) {
    return make_float3(
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    );
}

__device__ inline float3 interp(float3 p1, float3 p2, float v1, float v2, float isoLevel) {
    if (abs(v1 - v2) < 0.0001f)
        return p1;

    const float t = (isoLevel - v1) / (v2 - v1 + 1e-6f);

    return make_float3(
      p1.x + t * (p2.x - p1.x),
      p1.y + t * (p2.y - p1.y),
      p1.z + t * (p2.z - p1.z)
    );
}

__constant__ int3 offsets[8] = {
    {0,0,0},
    {1,0,0},
    {1,1,0},
    {0,1,0},
    {0,0,1},
    {1,0,1},
    {1,1,1},
    {0,1,1}
};

__global__ void marchingCubesKernel(
    const VoxelField field,
    const CudaAABB domain,
    int* vertexCount,
    int* triangleCount,
    Vertex* outVertices,
    unsigned int* outIndices,
    const int maxVertices,
    const int maxIndices,
    const float isoLevel,
    const bool smoothNormals
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int z = blockIdx.z * blockDim.z + threadIdx.z;

    if (x >= field.resolution.x - 1 || y >= field.resolution.y - 1 || z >= field.resolution.z - 1)
        return;

    int cx = field.resolution.x - 1;
    int cy = field.resolution.y - 1;
    int cz = field.resolution.z - 1;

    float3 cellSizes = make_float3(
        (domain.max.x - domain.min.x) / cx,
        (domain.max.y - domain.min.y) / cy,
        (domain.max.z - domain.min.z) / cz
    );

    // 1D scalar field offsets
    int dx = 1;
    int dy = field.resolution.x;
    int dz = field.resolution.x * field.resolution.y;
    int id = x + y * field.resolution.x + z * field.resolution.x * field.resolution.y;

    // Scalar field cube
    float cube[8];
    cube[0] = field.d_color[id               ];
    cube[1] = field.d_color[id + dx          ];
    cube[2] = field.d_color[id + dx + dy     ];
    cube[3] = field.d_color[id      + dy     ];
    cube[4] = field.d_color[id           + dz];
    cube[5] = field.d_color[id + dx      + dz];
    cube[6] = field.d_color[id + dx + dy + dz];
    cube[7] = field.d_color[id      + dy + dz];

    // Border
    for (int i = 0; i < 8; i++) {
        int vx = x + offsets[i].x;
        int vy = y + offsets[i].y;
        int vz = z + offsets[i].z;

        bool boundary =
            vx == 0 ||
            vy == 0 ||
            vz == 0 ||
            vx == field.resolution.x - 1 ||
            vy == field.resolution.y - 1 ||
            vz == field.resolution.z - 1;

        if (boundary) {
            cube[i] = isoLevel;
        }
    }
    // if (x == 0) {
    //     cube[0] = isoLevel;
    //     cube[3] = isoLevel;
    //     cube[4] = isoLevel;
    //     cube[7] = isoLevel;
    // }
    //
    // if (y == 0) {
    //     cube[0] = isoLevel;
    //     cube[1] = isoLevel;
    //     cube[4] = isoLevel;
    //     cube[5] = isoLevel;
    // }
    //
    // if (z == 0) {
    //     cube[0] = isoLevel;
    //     cube[1] = isoLevel;
    //     cube[2] = isoLevel;
    //     cube[3] = isoLevel;
    // }
    //
    // if (x == field.resolution.x - 2) {
    //     cube[1] = isoLevel;
    //     cube[2] = isoLevel;
    //     cube[5] = isoLevel;
    //     cube[6] = isoLevel;
    // }
    //
    // if (y == field.resolution.y - 2) {
    //     cube[2] = isoLevel;
    //     cube[3] = isoLevel;
    //     cube[6] = isoLevel;
    //     cube[7] = isoLevel;
    // }
    //
    // if (z == field.resolution.z - 2) {
    //     cube[4] = isoLevel;
    //     cube[5] = isoLevel;
    //     cube[6] = isoLevel;
    //     cube[7] = isoLevel;
    // }

    // Build cube index
    int cubeIndex = 0;
    if (cube[0] <= isoLevel) cubeIndex |= 1;
    if (cube[1] <= isoLevel) cubeIndex |= 2;
    if (cube[2] <= isoLevel) cubeIndex |= 4;
    if (cube[3] <= isoLevel) cubeIndex |= 8;
    if (cube[4] <= isoLevel) cubeIndex |= 16;
    if (cube[5] <= isoLevel) cubeIndex |= 32;
    if (cube[6] <= isoLevel) cubeIndex |= 64;
    if (cube[7] <= isoLevel) cubeIndex |= 128;

    // Skip empty / full cells
    if (cubeIndex == 0 || cubeIndex == 255)
        return;

    // Normals & Positions cube
    float3 normalsCube[8];
    normalsCube[0] = field.d_normal[id               ];
    normalsCube[1] = field.d_normal[id + dx          ];
    normalsCube[2] = field.d_normal[id + dx + dy     ];
    normalsCube[3] = field.d_normal[id      + dy     ];
    normalsCube[4] = field.d_normal[id           + dz];
    normalsCube[5] = field.d_normal[id + dx      + dz];
    normalsCube[6] = field.d_normal[id + dx + dy + dz];
    normalsCube[7] = field.d_normal[id      + dy + dz];

    float3 position[8];
    position[0] = make_float3((x + 0) * cellSizes.x, (y + 0) * cellSizes.y, (z + 0) * cellSizes.z) + domain.min;
    position[1] = make_float3((x + 1) * cellSizes.x, (y + 0) * cellSizes.y, (z + 0) * cellSizes.z) + domain.min;
    position[2] = make_float3((x + 1) * cellSizes.x, (y + 1) * cellSizes.y, (z + 0) * cellSizes.z) + domain.min;
    position[3] = make_float3((x + 0) * cellSizes.x, (y + 1) * cellSizes.y, (z + 0) * cellSizes.z) + domain.min;
    position[4] = make_float3((x + 0) * cellSizes.x, (y + 0) * cellSizes.y, (z + 1) * cellSizes.z) + domain.min;
    position[5] = make_float3((x + 1) * cellSizes.x, (y + 0) * cellSizes.y, (z + 1) * cellSizes.z) + domain.min;
    position[6] = make_float3((x + 1) * cellSizes.x, (y + 1) * cellSizes.y, (z + 1) * cellSizes.z) + domain.min;
    position[7] = make_float3((x + 0) * cellSizes.x, (y + 1) * cellSizes.y, (z + 1) * cellSizes.z) + domain.min;

    // Interpolate
    float3 edgeVertices[12];
    float3 edgeNormals[12];

    for (int i = 0 ; i < 12 ; i++) {
        if (edgeTable[cubeIndex] & (1 << i)) {
            int v0 = edgeToVertices[i][0];
            int v1 = edgeToVertices[i][1];

            edgeVertices[i] = interp(position[v0], position[v1], cube[v0], cube[v1], isoLevel);
            edgeNormals[i] = interp(normalsCube[v0], normalsCube[v1], cube[v0], cube[v1], isoLevel);
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

        // Compute face normals
        if (smoothNormals) {
            for (int j = 0 ; j < 3 ; j++) {
                int edgeIndex = triangleTable[cubeIndex][i + j];
                outVertices[baseVertexIndex + j].position = edgeVertices[edgeIndex];
                outVertices[baseVertexIndex + j].normal = edgeNormals[edgeIndex];
            }
        } else {
            int edgeIndex0 = triangleTable[cubeIndex][i + 0];
            int edgeIndex1 = triangleTable[cubeIndex][i + 1];
            int edgeIndex2 = triangleTable[cubeIndex][i + 2];
            float3 v0 = edgeVertices[edgeIndex0];
            float3 v1 = edgeVertices[edgeIndex1];
            float3 v2 = edgeVertices[edgeIndex2];
            float3 edge1 = make_float3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
            float3 edge2 = make_float3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);

            float3 faceNormal = make_float3(
                edge1.y * edge2.z - edge1.z * edge2.y,
                edge1.z * edge2.x - edge1.x * edge2.z,
                edge1.x * edge2.y - edge1.y * edge2.x
            );

            float length = sqrtf(faceNormal.x * faceNormal.x +
                                faceNormal.y * faceNormal.y +
                                faceNormal.z * faceNormal.z);
            if (length > 1e-6f) {
                faceNormal.x /= length;
                faceNormal.y /= length;
                faceNormal.z /= length;
            } else {
                faceNormal = make_float3(0.0f, 1.0f, 0.0f);
            }

            for (int j = 0 ; j < 3 ; j++) {
                int edgeIndex = triangleTable[cubeIndex][i + j];
                outVertices[baseVertexIndex + j].position = edgeVertices[edgeIndex];
                outVertices[baseVertexIndex + j].normal = faceNormal;
            }
        }

        outIndices[triangleIndex * 3 + 0] = baseVertexIndex + 0;
        outIndices[triangleIndex * 3 + 1] = baseVertexIndex + 1;
        outIndices[triangleIndex * 3 + 2] = baseVertexIndex + 2;
    }
}

void launchMarchingCubesKernel(
    const VoxelField field,
    const CudaAABB domain,
    int* vertexCount,
    int* triangleCount,
    Vertex* outVertices,
    unsigned int* outIndices,
    const int maxVertices,
    const int maxIndices,
    const float isoLevel,
    const bool smoothNormals
) {
    int threadsPerBlock = 256;
    int cells = (field.resolution.x - 1) * (field.resolution.y - 1) * (field.resolution.z - 1);
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    dim3 blockSize(8, 8, 8);
    dim3 gridSize(
        (field.resolution.x - 1 + blockSize.x - 1) / blockSize.x,
        (field.resolution.y - 1 + blockSize.y - 1) / blockSize.y,
        (field.resolution.z - 1 + blockSize.z - 1) / blockSize.z
    );

    marchingCubesKernel<<<gridSize, blockSize>>>(
        field, domain, vertexCount, triangleCount, outVertices, outIndices, maxVertices, maxIndices, isoLevel, smoothNormals
    );
}
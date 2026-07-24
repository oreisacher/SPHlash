#include "../../include/rendering/MarchingCubes.h"

#include "../../include/rendering/MarchingCubesLookup.h"
#include "assets/Vertex.h"
#include "glm/detail/func_geometric.inl"
#include "simulation/AABB.cuh"
#include "simulation/VoxelField.h"

__device__ inline glm::vec3 interp(glm::vec3 p1, glm::vec3 p2, float v1, float v2, float isoLevel) {
    if (abs(v1 - v2) < 0.0001f)
        return p1;

    const float t = (isoLevel - v1) / (v2 - v1 + 1e-6f);
    return p1 + t * (p2 - p1);
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
    const AABBView domain,
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

    glm::ivec3 cells = field.resolution - 1;
    glm::vec3 cellSizes = domain.size / static_cast<glm::vec3>(cells);

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
    glm::vec3 normalsCube[8];
    normalsCube[0] = field.d_normal[id               ];
    normalsCube[1] = field.d_normal[id + dx          ];
    normalsCube[2] = field.d_normal[id + dx + dy     ];
    normalsCube[3] = field.d_normal[id      + dy     ];
    normalsCube[4] = field.d_normal[id           + dz];
    normalsCube[5] = field.d_normal[id + dx      + dz];
    normalsCube[6] = field.d_normal[id + dx + dy + dz];
    normalsCube[7] = field.d_normal[id      + dy + dz];

    glm::vec3 position[8];
    position[0] = glm::vec3(x + 0, y + 0, z + 0) * cellSizes + domain.min;
    position[1] = glm::vec3(x + 1, y + 0, z + 0) * cellSizes + domain.min;
    position[2] = glm::vec3(x + 1, y + 1, z + 0) * cellSizes + domain.min;
    position[3] = glm::vec3(x + 0, y + 1, z + 0) * cellSizes + domain.min;
    position[4] = glm::vec3(x + 0, y + 0, z + 1) * cellSizes + domain.min;
    position[5] = glm::vec3(x + 1, y + 0, z + 1) * cellSizes + domain.min;
    position[6] = glm::vec3(x + 1, y + 1, z + 1) * cellSizes + domain.min;
    position[7] = glm::vec3(x + 0, y + 1, z + 1) * cellSizes + domain.min;

    // Interpolate
    glm::vec3 edgeVertices[12];
    glm::vec3 edgeNormals[12];

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

            glm::vec3 v0 = edgeVertices[edgeIndex0];
            glm::vec3 v1 = edgeVertices[edgeIndex1];
            glm::vec3 v2 = edgeVertices[edgeIndex2];

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;

            glm::vec3 faceNormal = edge1 * edge2 - edge1 * edge2;
            float length = glm::length(faceNormal);

            if (length > 1e-6f) {
                faceNormal.x /= length;
                faceNormal.y /= length;
                faceNormal.z /= length;
            } else {
                faceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
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
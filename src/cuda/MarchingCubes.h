//
// Created by ruut on 4/27/26.
//

#ifndef SPHLASH_MARCHINGCUBES_CU
#define SPHLASH_MARCHINGCUBES_CU

#include "rendering/Mesh.h"
#include "utils/AABB.h"
#include "utils/VoxelField.h"

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
);

#endif //SPHLASH_MARCHINGCUBES_CU

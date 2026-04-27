//
// Created by ruut on 4/27/26.
//

#ifndef SPHLASH_MARCHINGCUBES_CU
#define SPHLASH_MARCHINGCUBES_CU

#include "rendering/Mesh.h"

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
);

#endif //SPHLASH_MARCHINGCUBES_CU

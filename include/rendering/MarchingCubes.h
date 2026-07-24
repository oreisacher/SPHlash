//
// Created by ruut on 4/27/26.
//

#ifndef SPHLASH_MARCHINGCUBES_CU
#define SPHLASH_MARCHINGCUBES_CU

struct AABBView;
struct Vertex;
struct CudaAABB;
struct VoxelField;

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
);

#endif //SPHLASH_MARCHINGCUBES_CU

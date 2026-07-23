//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_VOXELFIELD_CUH
#define SPHLASH_VOXELFIELD_CUH

#include <vector_types.h>

struct CudaAABB;
struct VoxelField;

__global__ void voxelFieldKernel(
    VoxelField field,
    CudaAABB domain,
    int numParticles,
    const float4* position,
    const float4* properties,
    const int* hashes,
    const int* offsets,
    float h
);

#endif //SPHLASH_VOXELFIELD_CUH

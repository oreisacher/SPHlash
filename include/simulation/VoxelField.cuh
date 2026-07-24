//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_VOXELFIELD_CUH
#define SPHLASH_VOXELFIELD_CUH

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct AABBView;
struct VoxelField;

__global__ void voxelFieldKernel(
    VoxelField field,
    AABBView domain,
    int numParticles,
    const glm::vec3* position,
    const glm::vec4* properties,
    const int* hashes,
    const int* offsets,
    float h
);

#endif //SPHLASH_VOXELFIELD_CUH

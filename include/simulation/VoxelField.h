//
// Created by ruut on 5/3/26.
//

#ifndef SPHLASH_VOXELFIELD_H
#define SPHLASH_VOXELFIELD_H

#include <vector_types.h>

struct VoxelField {
    int3 resolution         = { 0, 0, 0 };

    float* d_color          = nullptr;
    float3* d_normal        = nullptr;
};

#endif //SPHLASH_VOXELFIELD_H

//
// Created by ruut on 5/3/26.
//

#ifndef SPHLASH_VOXELFIELD_H
#define SPHLASH_VOXELFIELD_H

#include "glm/vec3.hpp"

struct VoxelField {
    glm::ivec3 resolution         = { 0, 0, 0 };

    float* d_color          = nullptr;
    glm::vec3* d_normal        = nullptr;
};

#endif //SPHLASH_VOXELFIELD_H

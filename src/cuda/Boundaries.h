//
// Created by ruut on 4/20/26.
//

#ifndef SPHLASH_BOUNDARIES_H
#define SPHLASH_BOUNDARIES_H

#include "rendering/Mesh.h"
#include "utils/AABB.h"
#include "utils/Hex32Field.h"

void launchSdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    const Hex32Field& hexField,
    const CudaAABB& domain,
    const float h
);

void launchVolumeMapKernel(
    const Hex32Field sdfField,
    const Hex32Field volumeField,
    const CudaAABB domain
);

#endif //SPHLASH_BOUNDARIES_H

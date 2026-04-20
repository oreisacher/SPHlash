//
// Created by ruut on 4/20/26.
//

#ifndef SPHLASH_BOUNDARIES_H
#define SPHLASH_BOUNDARIES_H

#include "rendering/Mesh.h"

void launchSdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    float* sdf,
    const int cellCount,
    const int resolution,
    const float3 gridMin,
    const float3 gridMax
);

#endif //SPHLASH_BOUNDARIES_H

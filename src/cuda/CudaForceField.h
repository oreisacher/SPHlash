//
// Created by ruut on 6/13/26.
//

#ifndef SPHLASH_CUDAFORCEFIELD_H
#define SPHLASH_CUDAFORCEFIELD_H

#include <vector_types.h>

struct CudaForceField {
    float3 position     = { 0.0f, 0.0f, 0.0f };
    float3 halfExtents  = { 0.2f, 0.2f, 0.2f };

    float3 right        = { 1.0f, 0.0f, 0.0f };
    float3 up           = { 0.0f, 1.0f, 0.0f };
    float3 forward      = { 0.0f, 0.0f, 1.0f };

    bool active         = true;
    float strength      = 5.0f;
};

#endif //SPHLASH_CUDAFORCEFIELD_H

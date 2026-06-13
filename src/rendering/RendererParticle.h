//
// Created by ruut on 6/13/26.
//

#ifndef SPHLASH_RENDERERPARTICLE_H
#define SPHLASH_RENDERERPARTICLE_H

#include <vector_types.h>

struct RendererParticle {
    float4 position;
    float4 velocity;
    float4 properties;
    float4 flags;
    float4 color;
};

#endif //SPHLASH_RENDERERPARTICLE_H

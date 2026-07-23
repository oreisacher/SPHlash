//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_PARTICLEHOST_H
#define SPHLASH_PARTICLEHOST_H

#include <vector_types.h>

/*
    Represents a single particle inside simulation. Can be queried via getParticle(id). Host only
*/
struct ParticleHost {
    float4 position;
    float4 startPosition;
    float4 velocity;
    float4 properties;
    float4 flags;
    float4 color;
    float4 pressureForce;
    float4 viscosityForce;
    float4 surfaceTensionForce;
};

#endif //SPHLASH_PARTICLEHOST_H

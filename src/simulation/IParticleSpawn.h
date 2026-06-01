//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_IPARTICLESPAWN_H
#define SPHLASH_IPARTICLESPAWN_H

#include "FluidSimulation.h"

class IParticleSpawn {
public:
    virtual ~IParticleSpawn() = default;
    virtual CudaParticleSpawn getParticleSpawn() = 0;
};

#endif //SPHLASH_IPARTICLESPAWN_H

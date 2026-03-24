//
// Created by ruut on 3/20/26.
//

#ifndef FLUIDSIMSPH_HASHPARTICLES_H
#define FLUIDSIMSPH_HASHPARTICLES_H

#include <cstdint>

struct ParticleGPU;

void launchHashParticlesKernel(
    const ParticleGPU* particles,
    std::uint32_t* hashes,
    uint32_t* hashIndices,
    float cellSize,
    int numParticles
);

void sortIndicesByHash(
    uint32_t* hashes,
    uint32_t* hashIndices,
    int numParticles
);

void launchFindHashCellBoundariesKernel(
    const uint32_t* hashes,
    int32_t* cellStart,
    int32_t* cellEnd,
    int numParticles
);

void launchDensityPressureKernel(
    ParticleGPU* particles,
    const uint32_t* hashIndices,
    const int32_t* cellStart,
    const int32_t* cellEnd,
    float h,
    int numParticles,
    float k,
    float p_0
);

void launchSphForcesKernel(
    ParticleGPU* particles,
    const uint32_t* hashIndices,
    const int32_t* cellStart,
    const int32_t* cellEnd,
    float h,
    int numParticles,
    float mu,
    float tensionThreshold,
    float tension
);

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    int numParticles,
    float deltaTime,
    float width,
    float height,
    float gravity
);

void launchResetParticlesKernel(
    ParticleGPU* particles,
    int numParticles
);

#endif //FLUIDSIMSPH_HASHPARTICLES_H
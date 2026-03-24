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

void launchComputeCellIndexKernel(
    const int numParticles,
    ParticleGPU* particles,
    int* particleCellIndex,
    int* particleIndex,
    const float cellSize,
    const int2 gridResolution
);

void sortIndicesByHash(
    uint32_t* hashes,
    uint32_t* hashIndices,
    int numParticles
);

void sortIndicesByCellId(
    const int numParticles,
    int* particleCellIndex,
    int* particleIndex
);

void launchFindHashCellBoundariesKernel(
    const uint32_t* hashes,
    int32_t* cellStart,
    int32_t* cellEnd,
    int numParticles
);

void launchFindCellStartEndKernel(
    const int numParticles,
    const int* particleCellIndex,
    int* cellStart,
    int* cellEnd
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

void launchDensityPressureFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int2 gridResolution,
    const float h,
    const float k,
    const float p_0
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

void launchSphForcesFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int2 gridResolution,
    const float h,
    const float mu,
    const float tensionThreshold,
    const float tension
);

void launchApplyMouseForceKernel(
    const int numParticles,
    ParticleGPU* particles,
    const float2 mousePosition,
    const float mouseRadius,
    const float mouseForceDirection
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
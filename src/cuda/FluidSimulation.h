//
// Created by ruut on 3/20/26.
//

#ifndef FLUIDSIMSPH_HASHPARTICLES_H
#define FLUIDSIMSPH_HASHPARTICLES_H

#include <cstdint>

struct ParticleGPU;

void launchComputeCellIndexKernel(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleCellIndex,
    int* particleIndex,
    const float cellSize,
    const int3 gridResolution
);

void sortIndicesByCellId(
    const int numParticles,
    int* particleCellIndex,
    int* particleIndex
);

void launchFindCellStartEndKernel(
    const int numParticles,
    const int* particleCellIndex,
    int* cellStart,
    int* cellEnd
);

void launchDensityPressureFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int3 gridResolution,
    const float h,
    const float k,
    const float p_0
);

void launchSphForcesFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int3 gridResolution,
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
    const int numParticles,
    const float deltaTime,
    const float2 x,
    const float2 y,
    const float2 z,
    const float gravity
);

void launchResetParticlesKernel(
    ParticleGPU* particles,
    int numParticles
);

#endif //FLUIDSIMSPH_HASHPARTICLES_H
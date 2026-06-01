//
// Created by ruut on 3/20/26.
//

#ifndef FLUIDSIMSPH_HASHPARTICLES_H
#define FLUIDSIMSPH_HASHPARTICLES_H
#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulation.h"

struct ParticleGPU;

void launchHashParticles(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleHashValue,
    int* particleIndex,
    const float h
);

void sortParticlesByHash(
    const int numParticles,
    void* sortTempStorage,
    size_t sortTempStorageBytes,
    const int* particleHashes,
    int* sortedParticleHashes,
    const int* particleIndices,
    int* sortedParticleIndices
);

void launchBuildOffsetsKernel(
    int numParticles,
    const int* sortedHashes,
    int* offsets
);

float getMaxVelocity(
    int numParticles,
    ParticleGPU* particles
);

float getMaxAcceleration(
    int numParticles,
    ParticleGPU* particles
);

void queryRequiredSortingStorage(
    const int numParticles,
    size_t &sortTempStorageBytes,
    const int* particleHashes,
    int* sortedParticleHashes,
    const int* particleIndices,
    int* sortedParticleIndices
);

void launchReorderParticlesKernel(
    const int numParticles,
    const ParticleGPU* particles,
    const int* particleIndices,
    ParticleGPU* sortedParticles
);

void launchDensityPressureKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field sdfMap,
    const Hex32Field volumeMap,
    const CudaAABB domain,
    const float h,
    const float cs,
    const float p_0
);

void launchSphForcesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field sdfMap,
    const Hex32Field volumeMap,
    const CudaAABB domain,
    const float h,
    const float alpha,
    const float cs,
    const float tension,
    const float p_0
);

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const CudaForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const CudaAABB& domain,
    const float gravity
);

void launchRenderPrepareKernel(
    const int numParticles,
    const ParticleGPU* particles,
    RendererParticle* rendererParticles
);

void launchInitParticlesKernel(
    CudaParticleSpawn spawn,
    ParticleGPU* particles,
    int* particleCount,
    const int3 gridCount
);

void launchUpdateParticleMassKernel(
    int numParticles,
    ParticleGPU* particles,
    float mass
);

void launchResetParticlesKernel(
    ParticleGPU* particles,
    int numParticles
);

void launchVoxelFieldKernel(
    const VoxelField& field,
    const CudaAABB& domain,
    const int numParticles,
    const ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h
);
#endif //FLUIDSIMSPH_HASHPARTICLES_H
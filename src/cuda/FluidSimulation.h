//
// Created by ruut on 3/20/26.
//

#ifndef FLUIDSIMSPH_HASHPARTICLES_H
#define FLUIDSIMSPH_HASHPARTICLES_H

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
    const float h,
    const float k,
    const float p_0
);

void launchSphForcesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h,
    const float mu,
    const float tensionThreshold,
    const float tension
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

void launchRenderPrepareKernel(
    const int numParticles,
    const ParticleGPU* particles,
    float4* positions
);

void launchResetParticlesKernel(
    ParticleGPU* particles,
    int numParticles
);

#endif //FLUIDSIMSPH_HASHPARTICLES_H
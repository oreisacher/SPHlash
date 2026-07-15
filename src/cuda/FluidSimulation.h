//
// Created by ruut on 3/20/26.
//

#ifndef FLUIDSIMSPH_HASHPARTICLES_H
#define FLUIDSIMSPH_HASHPARTICLES_H

#include "simulation/FluidSimulation.h"
#include "simulation/scene/IParticleSpawn.h"

struct ParticleGPU;

void launchHashParticlesKernel(
    const int numParticles,
    const float4* position,
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
    const ParticlesGPU particles,
    const int* particleIndices,
    ParticlesGPU sortedParticles
);

void launchDensityPressureKernel(
    const int numParticles,
    const float4* position,
    float4* properties,
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
    const float4* position,
    const float4* properties,
    const float4* velocity,
    float4* pressureForce,
    float4* viscosityForce,
    float4* surfaceTensionForce,
    float4* flags,
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
    const float4* pressureForce,
    const float4* viscosityForce,
    const float4* surfaceTensionForce,
    float4* velocity,
    float4* position,
    const int numParticles,
    const CudaForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const CudaAABB& domain,
    const float gravity
);

void launchRenderPrepareKernel(
    const int numParticles,
    const float4* position,
    const float4* velocity,
    const float4* properties,
    const float4* flags,
    const float4* color,
    RendererParticle* rendererParticles
);

void launchInitParticlesKernel(
    CudaParticleSpawn spawn,
    ParticlesGPU particles,
    int* particleCount,
    const CudaAABB domain,
    const int3 gridCount
);

void launchResetParticlesKernel(
    float4* position,
    const float4* startPosition,
    float4* velocity,
    float4* properties,
    float4* pressureForce,
    float4* viscosityForce,
    float4* surfaceTensionForce,
    int numParticles
);

void launchVoxelFieldKernel(
    const VoxelField& field,
    const CudaAABB& domain,
    const int numParticles,
    const float4* position,
    const float4* properties,
    const int* hashes,
    const int* offsets,
    const float h
);
#endif //FLUIDSIMSPH_HASHPARTICLES_H
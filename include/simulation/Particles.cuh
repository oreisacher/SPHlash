#ifndef FLUIDSIMSPH_PARTICLES_H
#define FLUIDSIMSPH_PARTICLES_H

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct AABBView;
struct ForceField;
struct RendererParticle;
struct Hex32Field;
struct ParticlesGPU;
struct ParticleSpawn;

__global__ void initParticlesKernel(
    ParticleSpawn spawn,
    ParticlesGPU particles,
    int* particleCount,
    bool hasBoundary,
    Hex32Field sdfMap,
    AABBView domain,
    glm::ivec3 gridCount,
    float h
);

__global__ void resetParticlesKernel(
    glm::vec3* position,
    const glm::vec3* startPosition,
    glm::vec3* velocity,
    glm::vec4* properties,
    glm::vec3* pressureForce,
    glm::vec3* viscosityForce,
    glm::vec3* surfaceTensionForce,
    int numParticles
);

__global__ void renderPrepareKernel(
    int numParticles,
    const glm::vec3* position,
    const glm::vec3* velocity,
    const glm::vec4* properties,
    const glm::vec4* flags,
    const glm::vec3* color,
    RendererParticle* rendererParticles
);

__global__ void hashParticlesKernel(
    int numParticles,
    const glm::vec3* position,
    int* particleHashValue,
    int* particleIndex,
    float supportRadius,
    int tableSize
);

__global__ void buildOffsetsKernel(
    int numParticles,
    const int* hashes,
    int* offsets
);

__global__ void reorderParticlesKernel(
    int numParticles,
    ParticlesGPU particles,
    const int* particleIndices,
    ParticlesGPU sortedParticles
);

__global__ void densityPressureKernel(
    int numParticles,
    const glm::vec3* position,
    glm::vec4* properties,
    const int* hashes,
    int tableSize,
    const int* offsets,
    bool hasBoundary,
    Hex32Field sdfMap,
    Hex32Field volumeMap,
    AABBView domain,
    float h, float supportRadius, float cs, float p_0
);

__global__ void sphForcesKernel(
    int numParticles,
    const glm::vec3* position,
    const glm::vec4* properties,
    const glm::vec3* velocity,
    glm::vec3* pressureForce,
    glm::vec3* viscosityForce,
    glm::vec3* surfaceTensionForce,
    glm::vec4* flags,
    const int* hashes,
    int tableSize,
    const int* offsets,
    bool hasBoundary,
    Hex32Field sdfMap,
    Hex32Field volumeMap,
    AABBView domain,
    float h, float supportRadius, float alpha, float cs, float tension, float p_0
);

__global__ void updateParticlesKernel(
    const glm::vec3* pressureForce,
    const glm::vec3* viscosityForce,
    const glm::vec3* surfaceTensionForce,
    glm::vec3* velocity,
    glm::vec3* position,
    int numParticles,
    const ForceField* forceFields,
    int numForceFields, float deltaTime, AABBView domain, float gravity
);

__global__ void topSurfaceParticlesKernel(
    int numParticles,
    const glm::vec3* position,
    glm::vec4* flags,
    const int* hashes, int tableSize, const int* offsets, float h, float supportRadius
);

#endif
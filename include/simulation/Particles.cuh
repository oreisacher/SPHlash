#ifndef FLUIDSIMSPH_PARTICLES_H
#define FLUIDSIMSPH_PARTICLES_H

#include <vector_types.h>

struct CudaForceField;
struct RendererParticle;
struct Hex32Field;
struct CudaAABB;
struct ParticlesGPU;
struct CudaParticleSpawn;

__global__ void initParticlesKernel(CudaParticleSpawn spawn, ParticlesGPU particles, int* particleCount, CudaAABB domain, int3 gridCount);
__global__ void resetParticlesKernel(float4* position, const float4* startPosition, float4* velocity, float4* properties, float4* pressureForce, float4* viscosityForce, float4* surfaceTensionForce, int numParticles);
__global__ void renderPrepareKernel(int numParticles, const float4* position, const float4* velocity, const float4* properties, const float4* flags, const float4* color, RendererParticle* rendererParticles);
__global__ void hashParticlesKernel(int numParticles, const float4* position, int* particleHashValue, int* particleIndex, float h);
__global__ void buildOffsetsKernel(int numParticles, const int* hashes, int* offsets);
__global__ void reorderParticlesKernel(int numParticles, ParticlesGPU particles, const int* particleIndices, ParticlesGPU sortedParticles);
__global__ void densityPressureKernel(int numParticles, const float4* position, float4* properties, const int* hashes, const int* offsets, bool hasBoundary, Hex32Field sdfMap, Hex32Field volumeMap, CudaAABB domain, float h, float cs, float p_0);
__global__ void sphForcesKernel(int numParticles, const float4* position, const float4* properties, const float4* velocity, float4* pressureForce, float4* viscosityForce, float4* surfaceTensionForce, float4* flags, const int* hashes, const int* offsets, bool hasBoundary, Hex32Field sdfMap, Hex32Field volumeMap, CudaAABB domain, float h, float alpha, float cs, float tension, float p_0);
__global__ void updateParticlesKernel(const float4* pressureForce, const float4* viscosityForce, const float4* surfaceTensionForce, float4* velocity, float4* position, int numParticles, const CudaForceField* forceFields, int numForceFields, float deltaTime, CudaAABB domain, float gravity);
__global__ void topSurfaceParticlesKernel(int numParticles, const float4* position, float4* flags, const int* hashes, const int* offsets, float h);

#endif
#include "FluidSimulation.h"

#include <complex>

#include "simulation/FluidSimulationGPU.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <math_constants.h>
#include <cub/cub.cuh>

// --- Operator overloads ---
#pragma region OperatorOverloads
__device__ float4 operator+(const float4& a, const float4& b) {
    return make_float4(
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        0.0f
    );
}

__device__ float4 operator-(const float4& a, const float4& b) {
    return make_float4(
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        0.0f
    );
}

__device__ float4& operator+=(float4& a, const float4& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

__device__ float4 operator/(const float4& v, float s) {
    return make_float4(
        v.x / s,
        v.y / s,
        v.z / s,
        0.0f
    );
}

__device__ float4 operator*(float s, const float4& v) {
    return make_float4(
        v.x * s,
        v.y * s,
        v.z * s,
        0.0f
    );
}

__device__ float4 operator*(const float4& v, float s) {
    return make_float4(
        v.x * s,
        v.y * s,
        v.z * s,
        0.0f
    );
}
#pragma endregion

// --- Kernels ---
#pragma region Kernels
__device__ float poly6Kernel(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    float h2 = h * h;

    if (r2 <= h2) {
        float x = h2 - r2;
        float h9 = h2 * h2 * h2 * h2 * h;
        return 315.0f / (64.0f * CUDART_PI_F * h9) * x * x * x;
    }

    return 0.0f;
}

__device__ float4 poly6KernelGradient(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    float h2 = h * h;

    if (r2 < h2) {
        float x = h2 - r2;
        float h9 = h2 * h2 * h2 * h2 * h;
        float factor = (-945.0f / (32.0f * CUDART_PI_F * h9)) * x * x;
        return make_float4(
            factor * dist.x,
            factor * dist.y,
            factor * dist.z,
            0.0
        );
    }

    return make_float4(0.0, 0.0, 0.0, 0.0);
}

__device__ float poly6KernelLaplacian(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    float h2 = h * h;

    if (r2 < h2) {
        float h9 = h2 * h2 * h2 * h2 * h;
        float term1 = 945.0f / (32.0f * CUDART_PI_F * h9);
        float term2 = h2 - r2;
        float term3 = 7.0f * r2 - 3.0f * h2;

        return term1 * term2 * term3;
    }

    return 0.0f;
}

__device__ float4 spikyKernelGradient(float4 dist, float h) {
    float r = sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

    if (r > 0.0f && r < h) {
        float x = h - r;
        float h6 = h * h * h * h * h * h;
        float factor = (-45.0f / (CUDART_PI_F * h6)) * x * x;
        return make_float4(
            factor * (dist.x / r),
            factor * (dist.y / r),
            factor * (dist.z / r),
            0.0f
        );
    }

    return make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

__device__ float viscosityLaplacianKernel(float4 dist, float h) {
    float r = sqrt(dist.x * dist.x + dist.y * dist.y);

    if (r <= h) {
        float h6 = h * h * h * h * h * h;
        return (45.0f / (CUDART_PI_F * h6)) * (h - r);
    }

    return 0.0f;
}
#pragma endregion

// --- Simulation Steps ---
#pragma region SimulationSteps
__device__ int computeSpatialHash(int gx, int gy, int gz, int hashTableSize) {
    const int p1 = 73856093;
    const int p2 = 19349663;
    const int p3 = 83492791;

    int hash = (gx * p1) ^ (gy * p2) ^ (gz * p3);

    return abs(hash) % hashTableSize;
}

__global__ void computeCellIndexKernel(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleCellIndex,
    int* particleIndex,
    const float cellSize,
    const int3 gridResolution
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int gx = particles[id].position.x / cellSize;
    int gy = particles[id].position.y / cellSize;
    int gz = particles[id].position.z / cellSize;

    gx = max(0, min(gx, gridResolution.x - 1));
    gy = max(0, min(gy, gridResolution.y - 1));
    gz = max(0, min(gz, gridResolution.z - 1));

    int cellIndex = gx +
                    gy * gridResolution.x +
                    gz * gridResolution.x * gridResolution.y;

    particleCellIndex[id] = cellIndex;
    particleIndex[id] = id;
}

__global__ void computeCellIndexHashKernel(
    const int numParticles,
    ParticleGPU* particles,
    int* particleHashValue,
    int* particleIndex,
    const float h,
    const int hashTableSize
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int gx = floor(particles[id].position.x / h);
    int gy = floor(particles[id].position.y / h);
    int gz = floor(particles[id].position.z / h);

    particleHashValue[id] = computeSpatialHash(gx, gy, gz, hashTableSize);
    particleIndex[id] = id;
}

__global__ void findCellStartEndKernel(
    const int numParticles,
    const int* particleCellIndex,
    int* cellStart,
    int* cellEnd
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    const int cell = particleCellIndex[id];

    if (id == 0) {
        cellStart[cell] = id;
    } else {
        int prevCell = particleCellIndex[id - 1];
        if (cell != prevCell) {
            cellStart[cell] = id;
            cellEnd[prevCell] = id;
        }
    }

    if (id == numParticles - 1) {
        cellEnd[cell] = id + 1;
    }
}

__global__ void reorderParticlesKernel(
    const int numParticles,
    const int* particleIndex,
    const ParticleGPU* particles,
    ParticleGPU* sortedParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    if (id >= numParticles) return;

    // Move particle to its sorted index
    sortedParticles[id] = particles[particleIndex[id]];
}

__global__ void densityPressureFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int3 gridResolution,
    const float h,
    const float k,
    const float p_0
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    int pi = particleIndex[id];
    float4 pos = particles[pi].position;

    int gx = pos.x / h;
    int gy = pos.y / h;
    int gz = pos.z / h;

    gx = max(0, min(gx, gridResolution.x - 1));
    gy = max(0, min(gy, gridResolution.y - 1));
    gz = max(0, min(gz, gridResolution.z - 1));

    float density = 0.0f;
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int nx = gx + dx;
                int ny = gy + dy;
                int nz = gz + dz;

                if (nx < 0 || ny < 0 || nz < 0 ||
                    nx >= gridResolution.x || ny >= gridResolution.y || nz >= gridResolution.z)
                    continue;

                int neighborCell =  nx +
                                    ny * gridResolution.x +
                                    nz * gridResolution.x * gridResolution.y;

                int start = cellStart[neighborCell];

                // Cell with no particles
                if (start == -1)
                    continue;

                int end = cellEnd[neighborCell];

                for (int j = start ; j < end; j++) {
                    int pj = particleIndex[j];

                    const float4 diff = make_float4(
                        pos.x - particles[pj].position.x,
                        pos.y - particles[pj].position.y,
                        pos.z - particles[pj].position.z,
                        0.0f
                    );

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        density += particles[pj].properties.x * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    particles[pi].properties.y = density;
    particles[pi].properties.z = k * (density - p_0);
    // particles[pi].properties.z = max(k * (density - p_0), 0.0);     // Check if allowed to max here
}

__global__ void densityPressureHashKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int hashTableSize,
    const float h,
    const float k,
    const float p_0
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // int pi = particleIndex[id];
    // float4 pos = particles[pi].position;
    float4 pos = particles[id].position;

    const int gx = floorf(pos.x / h);
    const int gy = floorf(pos.y / h);
    const int gz = floorf(pos.z / h);

    float density = 0.0f;
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int neighborHash = computeSpatialHash(gx + dx, gy + dy, gz + dz, hashTableSize);

                int start = cellStart[neighborHash];

                // Cell with no particles
                if (start == -1)
                    continue;

                int end = cellEnd[neighborHash];

                for (int j = start ; j < end; j++) {
                    // int pj = particleIndex[j];

                    const float4 diff = make_float4(
                        pos.x - particles[j].position.x,
                        pos.y - particles[j].position.y,
                        pos.z - particles[j].position.z,
                        0.0f
                    );

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        density += particles[j].properties.x * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    particles[id].properties.y = density;
    particles[id].properties.z = k * (density - p_0);
}

__global__ void sphForcesFixedGridKernel(
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
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    int pi = particleIndex[id];
    ParticleGPU* particle = &particles[pi];

    float4 pos = particle->position;

    int gx = pos.x / h;
    int gy = pos.y / h;
    int gz = pos.z / h;

    gx = max(0, min(gx, gridResolution.x - 1));
    gy = max(0, min(gy, gridResolution.y - 1));
    gz = max(0, min(gz, gridResolution.z - 1));

    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4 n = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float c = 0.0f;

    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int nx = gx + dx;
                int ny = gy + dy;
                int nz = gz + dz;

                if (nx < 0 || ny < 0 || nz < 0 ||
                    nx >= gridResolution.x || ny >= gridResolution.y || nz >= gridResolution.z)
                    continue;

                int neighborCell =  nx +
                                    ny * gridResolution.x +
                                    nz * gridResolution.x * gridResolution.y;

                int start = cellStart[neighborCell];

                // Cell with no particles
                if (start == -1)
                    continue;

                int end = cellEnd[neighborCell];

                for (int j = start ; j < end; j++) {
                    int pj = particleIndex[j];

                    // Skip self
                    if (pi == pj)
                        continue;

                    const float4 diff = make_float4(
                        pos.x - particles[pj].position.x,
                        pos.y - particles[pj].position.y,
                        pos.z - particles[pj].position.z,
                        0.0f
                    );

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        ParticleGPU* other = &particles[pj];

                        // Pressure
                        const float scalar = other->properties.x * ((particle->properties.z + other->properties.z) / (2.0f * other->properties.y));
                        particle->pressureForce += scalar * spikyKernelGradient(diff, h);

                        // Viscosity
                        const float4 viscTerm = other->properties.x * ((other->velocity - particle->velocity) / other->properties.y);
                        particle->viscosityForce += viscosityLaplacianKernel(diff, h) * viscTerm;

                        // Surface tension
                        n += other->properties.x * (1.0f / other->properties.y) * poly6KernelGradient(diff, h);
                        c += other->properties.x * (1.0f / other->properties.y) * poly6KernelLaplacian(diff, h);
                    }
                }
            }
        }
    }

    particle->pressureForce = -1.0f * particle->pressureForce;
    particle->viscosityForce = mu * particle->viscosityForce;
    particle->debugging.x = 0.0f;

    float lenN = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (lenN > tensionThreshold) {
        float4 normalizedN = n / (lenN + 1e-6f);
        particle->surfaceTensionForce = -1.0f * tension * c * normalizedN;
        particle->debugging.x = lenN;
    }
}

__global__ void sphForcesHashKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int hashTableSize,
    const float h,
    const float mu,
    const float tensionThreshold,
    const float tension
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // int pi = particleIndex[id];
    ParticleGPU* particle = &particles[id];

    float4 pos = particle->position;

    const int gx = floorf(pos.x / h);
    const int gy = floorf(pos.y / h);
    const int gz = floorf(pos.z / h);

    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4 n = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float c = 0.0f;

    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int neighborHash = computeSpatialHash(gx + dx, gy + dy, gz + dz, hashTableSize);

                int start = cellStart[neighborHash];

                // Cell with no particles
                if (start == -1)
                    continue;

                int end = cellEnd[neighborHash];

                for (int j = start ; j < end; j++) {
                    // int pj = particleIndex[j];

                    // Skip self
                    if (id == j)
                        continue;

                    const float4 diff = make_float4(
                        pos.x - particles[j].position.x,
                        pos.y - particles[j].position.y,
                        pos.z - particles[j].position.z,
                        0.0f
                    );

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        ParticleGPU* other = &particles[j];

                        float invDensity = 1.0f / (other->properties.y + 1e-6f);

                        // Pressure
                        const float scalar = other->properties.x * (particle->properties.z + other->properties.z) * 0.5f * invDensity;
                        particle->pressureForce += scalar * spikyKernelGradient(diff, h);

                        // Viscosity
                        const float4 viscTerm = other->properties.x * (other->velocity - particle->velocity) * invDensity;
                        particle->viscosityForce += viscosityLaplacianKernel(diff, h) * viscTerm;

                        // Surface tension
                        n += other->properties.x * invDensity * poly6KernelGradient(diff, h);
                        c += other->properties.x * invDensity * poly6KernelLaplacian(diff, h);
                    }
                }
            }
        }
    }

    particle->pressureForce = -1.0f * particle->pressureForce;
    particle->viscosityForce = mu * particle->viscosityForce;
    particle->debugging.x = 0.0f;

    float lenN = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (lenN > tensionThreshold) {
        float4 normalizedN = n / (lenN + 1e-6f);
        particle->surfaceTensionForce = -1.0f * tension * c * normalizedN;
        particle->debugging.x = lenN;
    }
}

__global__ void applyMouseForceKernel(
    const int numParticles,
    ParticleGPU* particles,
    const float2 mousePosition,
    const float mouseRadius,
    const float mouseForceDirection
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    const float4 dir = make_float4(
        particle->position.x - mousePosition.x,
        particle->position.y - mousePosition.y,
        0.0f, 0.0f
    );

    const float len2 = dir.x * dir.x + dir.y * dir.y;

    if (len2 < mouseRadius * mouseRadius) {
        const float len = sqrtf(len2);
        const float4 forceDir = (len > 0.0001f) ? dir / len : make_float4(0.0f, 0.0f, 0.0f, 0.0f);
        const float falloff = 1.0f - (len / mouseRadius);
        particle->velocity += mouseForceDirection * mouseRadius * falloff * forceDir;
    }
}

__global__ void updateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const float deltaTime,
    const float2 x,
    const float2 y,
    const float2 z,
    const float gravity
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    // Update position
    const float4 force = particle->pressureForce + particle->viscosityForce + particle->surfaceTensionForce;
    const float4 acceleration = force / particle->properties.y + make_float4(0.0f, gravity, 0.0f, 1.0f);

    particle->velocity += (0.5f * deltaTime) * acceleration;
    particle->position += deltaTime * particle->velocity;
    particle->velocity += (0.5f * deltaTime) * acceleration;

    // Simple boundaries
    if (particle->position.x < x.x) {
        particle->position.x = x.x;
        particle->velocity.x *= -1.0f;
    }

    if (particle->position.x > x.y) {
        particle->position.x = x.y;
        particle->velocity.x *= -1.0f;
    }

    if (particle->position.y < y.x) {
        particle->position.y = y.x;
        particle->velocity.y *= -1.0f;
    }

    if (particle->position.y > y.y) {
        particle->position.y = y.y;
        particle->velocity.y *= -1.0f;
    }

    if (particle->position.z < z.x) {
        particle->position.z = z.x;
        particle->velocity.z *= -1.0f;
    }

    if (particle->position.z > z.y) {
        particle->position.z = z.y;
        particle->velocity.z *= -1.0f;
    }
}

__global__ void resetParticlesKernel(
    ParticleGPU* particles,
    const int numParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];
    particle->position = particle->startPosition;
    particle->velocity = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}
#pragma endregion

// --- Launchers ---
#pragma region Launchers
void launchComputeCellIndexKernel(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleCellIndex,
    int* particleIndex,
    const float cellSize,
    const int3 gridResolution
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    computeCellIndexKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleCellIndex, particleIndex, cellSize, gridResolution
    );
}

void launchComputeCellIndexHashKernel(
    const int numParticles,
    ParticleGPU* particles,
    int* particleHashValue,
    int* particleIndex,
    const float h,
    const int hashTableSize
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    computeCellIndexHashKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleHashValue, particleIndex, h, hashTableSize
    );
}

void queryCubRadixBytes(
    const int numParticles,
    size_t &sortTempStorageBytes,
    int* particleCellIndex1,
    int* particleCellIndex2,
    int* particleIndex1,
    int* particleIndex2
) {
    cub::DeviceRadixSort::SortPairs(
        nullptr,
        sortTempStorageBytes,
        particleCellIndex1, particleCellIndex2,
        particleIndex1, particleIndex2, numParticles
    );
}

void sortIndicesByCellId(
    const int numParticles,
    void* sortTempStorage,
    size_t sortTempStorageBytes,
    int* particleCellIndex1,
    int* particleCellIndex2,
    int* particleIndex1,
    int* particleIndex2
) {
    cub::DeviceRadixSort::SortPairs(
        sortTempStorage,
        sortTempStorageBytes,
        particleCellIndex1, particleCellIndex2,
        particleIndex1, particleIndex2, numParticles
    );
}

void launchReorderParticlesKernel(
    const int numParticles,
    const int* particleIndex,
    const ParticleGPU* particles,
    ParticleGPU* sortedParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    reorderParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particleIndex, particles, sortedParticles
    );
}

void launchFindCellStartEndKernel(
    const int numParticles,
    const int* particleCellIndex,
    int* cellStart,
    int* cellEnd
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    findCellStartEndKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particleCellIndex, cellStart, cellEnd
    );
}

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
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureFixedGridKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndex, cellStart, cellEnd, gridResolution, h, k, p_0
    );
}

void launchDensityPressureHashKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int hashTableSize,
    const float h,
    const float k,
    const float p_0
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureHashKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndex, cellStart, cellEnd, hashTableSize, h, k, p_0
    );
}

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
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesFixedGridKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndex, cellStart, cellEnd, gridResolution, h, mu, tensionThreshold, tension
    );
}

void launchSphForcesHashKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int hashTableSize,
    const float h,
    const float mu,
    const float tensionThreshold,
    const float tension
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesHashKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndex, cellStart, cellEnd, hashTableSize, h, mu, tensionThreshold, tension
    );
}

void launchApplyMouseForceKernel(
    const int numParticles,
    ParticleGPU* particles,
    const float2 mousePosition,
    const float mouseRadius,
    const float mouseForceDirection
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    applyMouseForceKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, mousePosition, mouseRadius, mouseForceDirection
    );
}

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const float deltaTime,
    const float2 x,
    const float2 y,
    const float2 z,
    const float gravity
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    updateParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles, deltaTime, x, y, z, gravity
    );
}

void launchResetParticlesKernel(
    ParticleGPU* particles,
    const int numParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    resetParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles
    );
}
#pragma endregion
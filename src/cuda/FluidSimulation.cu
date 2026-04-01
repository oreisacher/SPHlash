#include "FluidSimulation.h"
#include "simulation/FluidSimulation.h"
#include "simulation/FluidSimulationGPU.h"
#include "cuda/Kernels.cuh"
#include "cuda/Math.cuh"

#include <complex>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <cub/cub.cuh>

void queryRequiredSortingStorage(
    const int numParticles,
    size_t &sortTempStorageBytes,
    const int* particleHashes,
    int* sortedParticleHashes,
    const int* particleIndices,
    int* sortedParticleIndices
) {
    cub::DeviceRadixSort::SortPairs(
        nullptr,
        sortTempStorageBytes,
        particleHashes, sortedParticleHashes,
        particleIndices, sortedParticleIndices, numParticles
    );
}

// From SebLague. Find better explanation for paper
__device__ uint hashCell(int cellX, int cellY, int cellZ, int hashTableSize) {
    const uint blockSize = 50;
    const uint hash1 = 15823;
    const uint hash2 = 9737333;
    const uint hash3 = 440817757;

    // Shift cell by blocksize. Positions below -25 may produce unexpected behavior
    uint3 uCell = make_uint3(
        cellX + blockSize / 2,
        cellY + blockSize / 2,
        cellZ + blockSize / 2
    );

    // Calculate local position within the current block
    uint3 localCell = uCell % blockSize;

    // Calculate coordinate of current block itself
    uint3 blockID = uCell / blockSize;

    uint blockHash = blockID.x * hash1 + blockID.y * hash2 + blockID.z * hash3;
    return (localCell.x + blockSize * (localCell.y + blockSize * localCell.z) + blockHash) % hashTableSize;
}

__global__ void hashParticles(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleHashValue,
    int* particleIndex,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int cellX = floor(particles[id].position.x / h);
    int cellY = floor(particles[id].position.y / h);
    int cellZ = floor(particles[id].position.z / h);

    uint hash = hashCell(cellX, cellY, cellZ, numParticles);

    particleHashValue[id] = hash;
    particleIndex[id] = id;
}

void launchHashParticles(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleHashValue,
    int* particleIndex,
    const float h
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    hashParticles<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleHashValue, particleIndex, h
    );
}

void sortParticlesByHash(
    const int numParticles,
    void* sortTempStorage,
    size_t sortTempStorageBytes,
    const int* particleHashes,
    int* sortedParticleHashes,
    const int* particleIndices,
    int* sortedParticleIndices
) {
    cub::DeviceRadixSort::SortPairs(
        sortTempStorage,
        sortTempStorageBytes,
        particleHashes, sortedParticleHashes,
        particleIndices, sortedParticleIndices, numParticles
    );
}

__global__ void buildOffsetsKernel(
    const int numParticles,
    const int* hashes,
    int* offsets
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    const uint hash = hashes[id];
    const uint prevHash = id == 0 ? numParticles : hashes[id - 1];

    if (hash != prevHash) {
        offsets[hash] = id;
    }
}

void launchBuildOffsetsKernel(
    const int numParticles,
    const int* sortedHashes,
    int* offsets
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    buildOffsetsKernel<<<blocks, threadsPerBlock>>>(
        numParticles, sortedHashes, offsets
    );
}

__global__ void reorderParticlesKernel(
    const int numParticles,
    const ParticleGPU* particles,
    const int* particleIndices,
    ParticleGPU* sortedParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    if (id >= numParticles) return;

    // Move particle to its sorted index
    sortedParticles[id] = particles[particleIndices[id]];
}

void launchReorderParticlesKernel(
    const int numParticles,
    const ParticleGPU* particles,
    const int* particleIndices,
    ParticleGPU* sortedParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    reorderParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndices, sortedParticles
    );
}

__global__ void densityPressureKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h,
    const float k,
    const float p_0
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    float4 pos = particles[id].position;

    // Get cell of particle
    int cellX = floor(pos.x / h);
    int cellY = floor(pos.y / h);
    int cellZ = floor(pos.z / h);

    float density = 0.0f;
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cellX + dx, cellY + dy, cellZ + dz, numParticles);
                int offset = offsets[hash];

                if (offset == -1)
                    continue;

                while (offset < numParticles) {
                    int neighborIndex = offset;
                    offset++;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    const ParticleGPU other = particles[neighborIndex];
                    const float4 diff = pos - other.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        density += other.properties.x * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    particles[id].properties.y = density;
    particles[id].properties.z = k * (density - p_0);
}

void launchDensityPressureKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h,
    const float k,
    const float p_0
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, hashes, offsets, h, k, p_0
    );
}

__global__ void sphForcesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
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

    // Get cell of particle
    int cellX = floor(pos.x / h);
    int cellY = floor(pos.y / h);
    int cellZ = floor(pos.z / h);

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
                uint hash = hashCell(cellX + dx, cellY + dy, cellZ + dz, numParticles);
                int offset = offsets[hash];

                if (offset == -1)
                    continue;

                while (offset < numParticles) {
                    int neighborIndex = offset;
                    offset++;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    // Skip self
                    if (neighborIndex == id)
                        continue;

                    const ParticleGPU other = particles[neighborIndex];
                    const float4 diff = pos - other.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        float invDensity = 1.0f / (other.properties.y + 1e-6f);

                        // Pressure
                        const float scalar = other.properties.x * (particle->properties.z + other.properties.z) * 0.5f * invDensity;
                        particle->pressureForce += scalar * spikyKernelGradient(diff, h);

                        // Viscosity
                        const float4 viscTerm = other.properties.x * (other.velocity - particle->velocity) * invDensity;
                        particle->viscosityForce += viscosityLaplacianKernel(diff, h) * viscTerm;

                        // Surface tension
                        n += other.properties.x * invDensity * poly6KernelGradient(diff, h);
                        c += other.properties.x * invDensity * poly6KernelLaplacian(diff, h);
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

void launchSphForcesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h,
    const float mu,
    const float tensionThreshold,
    const float tension
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, hashes, offsets, h, mu, tensionThreshold, tension
    );
}

struct VelocitySq {
    __device__ float operator()(const ParticleGPU& p) const {
        return p.velocity.x * p.velocity.x +
               p.velocity.y * p.velocity.y +
               p.velocity.z * p.velocity.z;
    }
};

float getMaxVelocity(
    const int numParticles,
    ParticleGPU* particles
) {
    const thrust::device_ptr<ParticleGPU> begin(particles);
    const thrust::device_ptr<ParticleGPU> end(particles + numParticles);

    const float maxVal = thrust::transform_reduce(
        begin,
        end,
        VelocitySq(),
        0.0f,
        thrust::maximum<float>()
    );

    return sqrtf(maxVal);
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
        particle->velocity.x *= -0.9f;
    }

    if (particle->position.x > x.y) {
        particle->position.x = x.y;
        particle->velocity.x *= -0.9f;
    }

    if (particle->position.y < y.x) {
        particle->position.y = y.x;
        particle->velocity.y *= -0.9f;
    }

    if (particle->position.y > y.y) {
        particle->position.y = y.y;
        particle->velocity.y *= -0.9f;
    }

    if (particle->position.z < z.x) {
        particle->position.z = z.x;
        particle->velocity.z *= -0.9f;
    }

    if (particle->position.z > z.y) {
        particle->position.z = z.y;
        particle->velocity.z *= -0.9f;
    }
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
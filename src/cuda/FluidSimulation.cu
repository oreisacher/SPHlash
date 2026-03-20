#include "FluidSimulation.h"
#include "simulation/FluidSimulationGPU.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <math_constants.h>

__global__ void hashParticlesKernel(
    ParticleGPU* particles,
    uint32_t* hashes,
    uint32_t* hashIndices,
    float cellSize,
    int numParticles,
    int hashTableSize
) {
    uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int2 cell = make_int2(
        static_cast<int>(floorf((particles[id].position.x + 1000.0f) / cellSize)),
        static_cast<int>(floorf((particles[id].position.y + 1000.0f) / cellSize))
    );

    // Calc hash of cell. Compute with signed integers to avoid negative cell coords problems > Convert to uint later
    uint32_t hash = static_cast<uint32_t>(cell.x * 73856093) ^ static_cast<uint32_t>(cell.y * 19349663);
    hash %= hashTableSize;

    hashes[id] = hash;
    hashIndices[id] = id;
}

void launchHashParticlesKernel(
    ParticleGPU* particles,
    uint32_t* hashes,
    uint32_t* hashIndices,
    float cellSize,
    int numParticles,
    int hashTableSize
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    hashParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, hashes, hashIndices, cellSize, numParticles, hashTableSize
    );

    cudaDeviceSynchronize();
}

void sortIndicesByHash(
    uint32_t* hashes,
    uint32_t* hashIndices,
    int numParticles
) {
    thrust::device_ptr<uint32_t> d_keys_ptr(hashes);
    thrust::device_ptr<uint32_t> d_values_ptr(hashIndices);

    thrust::sort_by_key(
        d_keys_ptr,
        d_keys_ptr + numParticles,
        d_values_ptr
    );

    cudaDeviceSynchronize();
}

__global__ void findHashCellBoundariesKernel(
    uint32_t* hashes,
    int32_t* cellStart,
    int32_t* cellEnd,
    int numParticles
) {
    uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    uint32_t currentHash = hashes[id];

    // Check if first hash or the prev hash is different
    if (id == 0 || currentHash != hashes[id - 1]) {
        // Mark start of new hash area
        cellStart[currentHash] = id;

        // Mark end of previous hash area
        if (id > 0) {
            cellEnd[hashes[id - 1]] = id;
        }
    }

    if (id == numParticles - 1) {
        cellEnd[currentHash] = id + 1;
    }
}

void launchFindHashCellBoundariesKernel(
    uint32_t* hashes,
    int32_t* cellStart,
    int32_t* cellEnd,
    int numParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    findHashCellBoundariesKernel<<<blocks, threadsPerBlock>>>(
        hashes, cellStart, cellEnd, numParticles
    );

    cudaDeviceSynchronize();
}

__device__ float poly6Kernel2D(float2 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y;
    float h2 = h * h;

    if (r2 <= h2) {
        float x = h2 - r2;
        float h8 = h2 * h2 * h2 * h2;
        return 4.0f / (CUDART_PI_F * h8) * x * x * x;
    }

    return 0.0f;
}

__global__ void densityPressureKernel(
    ParticleGPU* particles,
    uint32_t* hashIndices,
    int32_t* cellStart,
    int32_t* cellEnd,
    float h,
    int numParticles,
    float k,
    float p_0
) {
    uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get current cell of particle
    int2 cell = make_int2(
        static_cast<int>(floorf((particles[id].position.x + 1000.0f) / h)),
        static_cast<int>(floorf((particles[id].position.y + 1000.0f) / h))
    );

    float density = 0.0f;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            // Calculate neighbor hash
            int2 neighborCell = make_int2(cell.x + x, cell.y + y);
            uint32_t hash = static_cast<uint32_t>(neighborCell.x * 73856093) ^ static_cast<uint32_t>(neighborCell.y * 19349663);
            hash %= numParticles;

            int start = cellStart[hash];

            if (start == -1)
                continue; // Cell empty

            int end = cellEnd[hash];

            for (int j = start ; j < end; j++) {
                int neighborId = hashIndices[j];

                float2 diff = make_float2(
                    particles[id].position.x - particles[neighborId].position.x,
                    particles[id].position.y - particles[neighborId].position.y
                );

                // Check if within radius
                float distSq = diff.x * diff.x + diff.y * diff.y;
                if (distSq < h * h) {
                    density += particles[neighborId].properties.x * poly6Kernel2D(diff, h);
                }
            }
        }
    }

    particles[id].properties.y = density;
    particles[id].properties.z = k * (particles[id].properties.y - p_0);
}

void launchDensityPressureKernel(
    ParticleGPU* particles,
    uint32_t* hashIndices,
    int32_t* cellStart,
    int32_t* cellEnd,
    float h,
    int numParticles,
    float k,
    float p_0
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureKernel<<<blocks, threadsPerBlock>>>(
        particles, hashIndices, cellStart, cellEnd, h, numParticles, k, p_0
    );

    cudaDeviceSynchronize();
}
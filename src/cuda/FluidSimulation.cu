#include "FluidSimulation.h"

#include <complex>

#include "simulation/FluidSimulationGPU.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <math_constants.h>

// Overloads
__device__ float4 operator+(const float4& a, const float4& b) {
    return make_float4(
        a.x + b.x,
        a.y + b.y,
        0.0f,
        0.0f
    );
}

__device__ float4 operator-(const float4& a, const float4& b) {
    return make_float4(
        a.x - b.x,
        a.y - b.y,
        0.0f,
        0.0f
    );
}

__device__ float4& operator+=(float4& a, const float4& b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

__device__ float4 operator/(const float4& v, float s) {
    return make_float4(
        v.x / s,
        v.y / s,
        0.0f,
        0.0f
    );
}

__device__ float4 operator*(float s, const float4& v) {
    return make_float4(
        v.x * s,
        v.y * s,
        0.0f,
        0.0f
    );
}

// ---

__global__ void hashParticlesKernel(
    ParticleGPU* particles,
    uint32_t* hashes,
    uint32_t* hashIndices,
    float cellSize,
    int numParticles
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
    hash %= numParticles;

    hashes[id] = hash;
    hashIndices[id] = id;
}

void launchHashParticlesKernel(
    ParticleGPU* particles,
    uint32_t* hashes,
    uint32_t* hashIndices,
    float cellSize,
    int numParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    hashParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, hashes, hashIndices, cellSize, numParticles
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

__device__ float4 poly6Kernel2DGradient(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y;
    float h2 = h * h;

    if (r2 > 0 && r2 < h2) {
        float x = h2 - r2;
        float h8 = h2 * h2 * h2 * h2;
        float factor = -24.0f / (CUDART_PI_F * h8) * x * x;
        return make_float4(factor * dist.x, factor * dist.y, 0.0f, 0.0f);
    }

    return make_float4(0.0, 0.0, 0.0, 0.0);
}

__device__ float poly6Kernel2DLaplacian(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y;
    float h2 = h * h;

    if (r2 > 0 && r2 < h2) {
        float h8 = h2 * h2 * h2 * h2;
        float term1 = 24.0f / (CUDART_PI_F * h8);
        float term2 = h2 - r2;
        float term3 = 4.0f * r2 - h2;

        return term1 * term2 * term3;
    }

    return 0.0f;
}

__device__ float4 spikyKernel2DGradient(float2 dist, float h) {
    float r = sqrt(dist.x * dist.x + dist.y * dist.y);

    if (r > 0 && r < h) {
        float x = h - r;
        float h5 = h * h * h * h * h;
        float factor = (-30.0f / (CUDART_PI_F * h5)) * x * x;
        return make_float4(
            factor * (dist.x / r),
            factor * (dist.y / r),
            0.0f,
            0.0f
        );
    }

    return make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

__device__ float viscosityLaplacianKernel2D(float4 dist, float h) {
    float r = sqrt(dist.x * dist.x + dist.y * dist.y);

    if (r <= h) {
        float h6 = h * h * h * h * h * h;
        return (45.0f / (CUDART_PI_F * h6)) * (h - r);
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

__global__ void sphForcesKernel(
    ParticleGPU* particles,
    uint32_t* hashIndices,
    int32_t* cellStart,
    int32_t* cellEnd,
    float h,
    int numParticles,
    float mu,
    float tension
) {
    uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Reset old values
    ParticleGPU* particle = &particles[id];
    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4 n = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float c = 0.0f;

    // Get current cell of particle
    int2 cell = make_int2(
        static_cast<int>(floorf((particle->position.x + 1000.0f) / h)),
        static_cast<int>(floorf((particle->position.y + 1000.0f) / h))
    );

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

                if (id == neighborId)
                    continue;

                float2 diff = make_float2(
                    particle->position.x - particles[neighborId].position.x,
                    particle->position.y - particles[neighborId].position.y
                );

                // Check if within radius
                float distSq = diff.x * diff.x + diff.y * diff.y;
                if (distSq < h * h) {
                    ParticleGPU* other = &particles[neighborId];

                    // Pressure
                    const float scalar = other->properties.x * ((particle->properties.z + other->properties.z) / (2.0f * other->properties.y));
                    particle->pressureForce += scalar * spikyKernel2DGradient(diff, h);

                    // Viscosity
                    const float4 viscTerm = other->properties.x * ((other->velocity - particle->velocity) / other->properties.y);
                    particle->viscosityForce += viscosityLaplacianKernel2D(particle->position - other->position, h) * viscTerm;

                    // Surface tension
                    n += other->properties.x * (1.0f / other->properties.y) * poly6Kernel2DGradient(particle->position - other->position, h);
                    c += other->properties.x * (1.0f / other->properties.y) * poly6Kernel2DLaplacian(particle->position - other->position, h);
                }
            }
        }
    }

    particle->pressureForce = -1.0f * particle->pressureForce;
    particle->viscosityForce = mu * particle->viscosityForce;

    float lenN = sqrtf(n.x * n.x + n.y * n.y);
    if (lenN > 10.0f) {
        particle->surfaceTensionForce = -1.0f * tension * c * (n / lenN);
    }
}

void launchSphForcesKernel(
    ParticleGPU* particles,
    uint32_t* hashIndices,
    int32_t* cellStart,
    int32_t* cellEnd,
    float h,
    int numParticles,
    float mu,
    float tension
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesKernel<<<blocks, threadsPerBlock>>>(
        particles, hashIndices, cellStart, cellEnd, h, numParticles, mu, tension
    );

    cudaDeviceSynchronize();
}

__global__ void updateParticlesKernel(
    ParticleGPU* particles,
    int numParticles,
    float deltaTime,
    float width,
    float height,
    float gravity
) {
    uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    // Update position
    float4 force = particle->pressureForce + particle->viscosityForce + particle->surfaceTensionForce;
    float4 acceleration = force / particle->properties.y + make_float4(0.0f, gravity, 0.0f, 1.0f);

    particle->velocity += (0.5f * deltaTime) * acceleration;
    particle->position += deltaTime * particle->velocity;
    particle->velocity += (0.5f * deltaTime) * acceleration;

    // Simple boundaries
    if (particle->position.x < 0) {
        particle->position.x = 0;
        particle->velocity.x *= -1.0f;
    }

    if (particle->position.x > width) {
        particle->position.x = width;
        particle->velocity.x *= -1.0f;
    }

    if (particle->position.y < 0) {
        particle->position.y = 0;
        particle->velocity.y *= -1.0f;
    }

    if (particle->position.y > height) {
        particle->position.y = height;
        particle->velocity.y *= -1.0f;
    }
}

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    int numParticles,
    float deltaTime,
    float width,
    float height,
    float gravity
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    updateParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles, deltaTime, width, height, gravity
    );

    cudaDeviceSynchronize();
}

__global__ void resetParticlesKernel(
    ParticleGPU* particles,
    int numParticles
) {
    uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];
    particle->position = particle->startPosition;
    particle->velocity = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void launchResetParticlesKernel(
    ParticleGPU* particles,
    int numParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    resetParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles
    );

    cudaDeviceSynchronize();
}
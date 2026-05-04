#include "FluidSimulation.h"
#include "simulation/FluidSimulation.h"
#include "simulation/FluidSimulation.h"
#include "cuda/Kernels.cuh"
#include "cuda/Math.cuh"

#include <complex>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <cub/cub.cuh>

#include "rendering/FluidRenderer.h"

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

__device__ int floorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;

    if ((r != 0) && ((r < 0) != (b < 0)))
        q--;

    return q;
}

__device__ uint floorMod(int a, int b) {
    int r = a % b;

    if ((r != 0) && ((r < 0) != (b < 0)))
        r += b;

    return (uint)r;
}

// From SebLague. Find better explanation for paper
__device__ uint hashCell(int cellX, int cellY, int cellZ, int hashTableSize) {
    const uint blockSize = 50;
    const uint hash1 = 15823;
    const uint hash2 = 9737333;
    const uint hash3 = 440817757;

    // Calculate local position within the current block
    uint3 localCell = make_uint3(
        floorMod(cellX, blockSize),
        floorMod(cellY, blockSize),
        floorMod(cellZ, blockSize)
    );

    // Calculate coordinate of current block itself
    int3 blockID = make_int3(
        floorDiv(cellX, blockSize),
        floorDiv(cellY, blockSize),
        floorDiv(cellZ, blockSize)
    );

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


    // const float B = (k * p_0) / 7;
    // particles[id].properties.z = B * (pow((density / p_0), 7) - 1);
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
                        // const float scalar = other.properties.x *
                            // (particle->properties.z / (particle->properties.y * particle->properties.y) +
                             // other.properties.z / (other.properties.y * other.properties.y));
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

    // Flag for surface particle
    particle->flags.x = 0.0f;

    float lenN = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (lenN > tensionThreshold) {
        float4 normalizedN = n / (lenN + 1e-6f);
        particle->surfaceTensionForce = -1.0f * tension * c * normalizedN;
        particle->flags.x = 1.0f;
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

__device__ float3 toLocal(float3 p, const ForceField& f) {
    // float3 d = p - f.position;
    //
    // return make_float3(
    //     dot(d, make_float3(f.right.x,   f.up.x,   f.forward.x)),
    //     dot(d, make_float3(f.right.y,   f.up.y,   f.forward.y)),
    //     dot(d, make_float3(f.right.z,   f.up.z,   f.forward.z))
    // );

    float3 d = p - f.position;

    return make_float3(
        dot(d, f.right),
        dot(d, f.up),
        dot(d, f.forward)
    );
}

__device__ bool isInsideField(float3 worldPos, const ForceField& f) {
    float3 p = toLocal(worldPos, f);

    return (fabsf(p.x) <= f.halfExtents.x &&
            fabsf(p.y) <= f.halfExtents.y &&
            fabsf(p.z) <= f.halfExtents.z);
}

__device__ float4 toWorldForce(float4 fLocal, const ForceField& f) {
    return make_float4(
        fLocal.x * f.right.x   + fLocal.y * f.up.x   + fLocal.z * f.forward.x,
        fLocal.x * f.right.y   + fLocal.y * f.up.y   + fLocal.z * f.forward.y,
        fLocal.x * f.right.z   + fLocal.y * f.up.z   + fLocal.z * f.forward.z,
        0.0
    );
}

__global__ void updateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const ForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const CudaAABB domain,
    const float gravity
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    // Update position
    const float4 force = particle->pressureForce + particle->viscosityForce + particle->surfaceTensionForce;
    float4 acceleration = force / particle->properties.y + make_float4(0.0f, gravity, 0.0f, 0.0f);

    // Check force fields
    for (int f = 0 ; f < numForceFields; f++) {
        if (isInsideField(make_float3(particle->position.x, particle->position.y, particle->position.z), forceFields[f])) {
            acceleration += toWorldForce({ forceFields[f].strength, 0.0, 0.0, 0.0 }, forceFields[f]);
        }
    }

    // TODO - Implement correctly
    // particle->velocity += (0.5f * deltaTime) * acceleration;
    // particle->position += deltaTime * particle->velocity;
    // particle->velocity += (0.5f * deltaTime) * acceleration;

    particle->velocity += deltaTime * acceleration;
    particle->position += deltaTime * particle->velocity;

    // Domain boundaries
    if (particle->position.x < domain.min.x) {
        particle->position.x = domain.min.x;
        particle->velocity.x *= -0.4f;
    }

    if (particle->position.x > domain.max.x) {
        particle->position.x = domain.max.x;
        particle->velocity.x *= -0.4f;
    }

    if (particle->position.y < domain.min.y) {
        particle->position.y = domain.min.y;
        particle->velocity.y *= -0.4f;
    }

    if (particle->position.y > domain.max.y) {
        particle->position.y = domain.max.y;
        particle->velocity.y *= -0.4f;
    }

    if (particle->position.z < domain.min.z) {
        particle->position.z = domain.min.z;
        particle->velocity.z *= -0.4f;
    }

    if (particle->position.z > domain.max.z) {
        particle->position.z = domain.max.z;
        particle->velocity.z *= -0.4f;
    }
}

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const ForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const CudaAABB& domain,
    const float gravity
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    updateParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles, forceFields, numForceFields, deltaTime, domain, gravity
    );
}

__global__ void renderPrepareKernel(
    const int numParticles,
    const ParticleGPU* particles,
    RendererParticle* rendererParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    rendererParticles[id].position = particles[id].position;
    rendererParticles[id].velocity = particles[id].velocity;
    rendererParticles[id].properties = particles[id].properties;
    rendererParticles[id].flags = particles[id].flags;
}

void launchRenderPrepareKernel(
    const int numParticles,
    const ParticleGPU* particles,
    RendererParticle* rendererParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    renderPrepareKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, rendererParticles
    );
}

__global__ void initParticlesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int3 gridCount,
    const float4 startPosition,
    const float particleSpacing,
    const float mass
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Map id to 3d position
    const int gx = id % gridCount.x;
    const int gy = (id / gridCount.x) % gridCount.y;
    const int gz = id / (gridCount.x * gridCount.y);

    float posX = startPosition.x + gx * particleSpacing;
    float posY = startPosition.y + gy * particleSpacing;
    float posZ = startPosition.z + gz * particleSpacing;

    ParticleGPU* particle = &particles[id];
    particle->position = { posX, posY, posZ, 0.0 };
    particle->startPosition = particle->position;
    particle->velocity = { 0.0, 0.0, 0.0, 0.0 };
    particle->properties = { mass, 0.0, 0.0, 0.0 };
    particle->flags = { 0.0, 0.0, 0.0, 0.0 };
    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void launchInitParticlesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int3 gridCount,
    const float4 startPosition,
    const float particleSpacing,
    const float mass
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    initParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, gridCount, startPosition, particleSpacing, mass
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
    particle->properties.y = 0.0f;
    particle->properties.z = 0.0f;
    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
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

__global__ void voxelFieldKernel(
    const VoxelField field,
    const CudaAABB domain,
    const int numParticles,
    const ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = field.resolution.x * field.resolution.y * field.resolution.z;

    if (id >= cellCount)
        return;

    // Convert index to 3d cell pos
    int x = id % field.resolution.x;
    int y = (id / field.resolution.x) % field.resolution.y;
    int z = id / (field.resolution.x * field.resolution.y);


    float3 cellSizes = make_float3(
        (domain.max.x - domain.min.x) / (field.resolution.x - 1),
        (domain.max.y - domain.min.y) / (field.resolution.y - 1),
        (domain.max.z - domain.min.z) / (field.resolution.z - 1)
    );

    float4 samplePosition = make_float4(
        domain.min.x + x * cellSizes.x,
        domain.min.y + y * cellSizes.y,
        domain.min.z + z * cellSizes.z,
        0.0
    );

    // Get cell of sample point
    int cellX = floor(samplePosition.x / h);
    int cellY = floor(samplePosition.y / h);
    int cellZ = floor(samplePosition.z / h);

    float4 n = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float c = 0.0f;
    // float h2 = field.smoothingRadius * field.smoothingRadius;
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

                    const ParticleGPU particle = particles[neighborIndex];
                    const float4 diff = samplePosition - particle.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        float invDensity = 1.0f / (particle.properties.y + 1e-6f);

                        // Color field
                        n += particle.properties.x * invDensity * poly6KernelGradient(diff, h);
                        c += particle.properties.x * invDensity * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    field.d_color[id] = c - 0.5f;
    field.d_normal[id] = make_float3(-n.x, -n.y, -n.z);
}

void launchVoxelFieldKernel(
    const VoxelField& field,
    const CudaAABB& domain,
    const int numParticles,
    const ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h
) {
    int threadsPerBlock = 256;
    int cells = field.resolution.x * field.resolution.y * field.resolution.z;
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    voxelFieldKernel<<<blocks, threadsPerBlock>>>(
        field, domain, numParticles, particles, hashes, offsets, h
    );
}
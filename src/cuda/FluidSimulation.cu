#include "FluidSimulation.h"
#include "simulation/FluidSimulation.h"
#include "cuda/Kernels.cuh"
#include "cuda/Math.cuh"

#include <complex>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <cub/cub.cuh>

#include "Hex32.cuh"
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

__device__ float ggamma(float restDensity, float h, float x) {
    if (x <= h)
        return restDensity * (1 - x / h);
    return 0;
}

__device__ float domainBoundaryDensity(float3 pos, CudaAABB domain, float restDensity, float h) {
    float density = 0.0f;

    // Distance to each face (positive = inside domain)
    float dxMin = pos.x - domain.min.x;
    float dxMax = domain.max.x - pos.x;
    float dyMin = pos.y - domain.min.y;
    float dyMax = domain.max.y - pos.y;
    float dzMin = pos.z - domain.min.z;
    float dzMax = domain.max.z - pos.z;

    // For a planar boundary the density contribution has an analytic form
    // gamma(phi) convolved with W along the half-space
    // Approximated as the same gamma ramp you use for mesh boundaries
    density += ggamma(restDensity, h, dxMin);
    density += ggamma(restDensity, h, dxMax);
    density += ggamma(restDensity, h, dyMin);
    density += ggamma(restDensity, h, dyMax);
    density += ggamma(restDensity, h, dzMin);
    density += ggamma(restDensity, h, dzMax);

    return density;
}

__global__ void densityPressureKernel(
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
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    float4 position = particles[id].position;

    // Get cell of particle
    int cellX = floor(position.x / h);
    int cellY = floor(position.y / h);
    int cellZ = floor(position.z / h);

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
                    const float4 diff = position - other.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        density += other.properties.x * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    // Handle boundary density
    if (hasBoundary) {
        float3 boundaryCellSize = make_float3(
            (domain.max.x - domain.min.x) / volumeMap.resolution.x,
            (domain.max.y - domain.min.y) / volumeMap.resolution.y,
            (domain.max.z - domain.min.z) / volumeMap.resolution.z
        );

        // Determine boundary cell coords and clamp to valid range
        int boundaryCellX = max(0, min(static_cast<int>((position.x - domain.min.x) / boundaryCellSize.x), volumeMap.resolution.x - 1));
        int boundaryCellY = max(0, min(static_cast<int>((position.y - domain.min.y) / boundaryCellSize.y), volumeMap.resolution.y - 1));
        int boundaryCellZ = max(0, min(static_cast<int>((position.z - domain.min.z) / boundaryCellSize.z), volumeMap.resolution.z - 1));

        int boundaryCellId = boundaryCellX
                        + boundaryCellY * volumeMap.resolution.x
                        + boundaryCellZ * volumeMap.resolution.x * volumeMap.resolution.y;

        Hex32Cell* sdfCell      = &sdfMap.d_cells[boundaryCellId];
        Hex32Cell* volumeCell   = &volumeMap.d_cells[boundaryCellId];

        // Cell does not contribute any density if unused, meaning the cell is far away from any boundary
        if (volumeCell->used) {
            float3 boundaryCellMin = make_float3(
                domain.min.x + boundaryCellX * boundaryCellSize.x,
                domain.min.y + boundaryCellY * boundaryCellSize.y,
                domain.min.z + boundaryCellZ * boundaryCellSize.z
            );

            float3 boundaryCellMax = make_float3(
                boundaryCellMin.x + boundaryCellSize.x,
                boundaryCellMin.y + boundaryCellSize.y,
                boundaryCellMin.z + boundaryCellSize.z
            );

            float3 isoparametricCoords = T(boundaryCellMin, boundaryCellMax, make_float3(position.x, position.y, position.z));
            isoparametricCoords.x = fmaxf(-1.0f, fminf(1.0f, isoparametricCoords.x));
            isoparametricCoords.y = fmaxf(-1.0f, fminf(1.0f, isoparametricCoords.y));
            isoparametricCoords.z = fmaxf(-1.0f, fminf(1.0f, isoparametricCoords.z));

            float interpolatedSDF = interpolateHex32(
                isoparametricCoords.x,
                isoparametricCoords.y,
                isoparametricCoords.z,
                sdfCell->nodes
            );

            float interpolatedVolume = interpolateHex32(
                isoparametricCoords.x,
                isoparametricCoords.y,
                isoparametricCoords.z,
                volumeCell->nodes
            );

            density += interpolatedVolume * p_0 * poly6Kernel(interpolatedSDF, h);
        }
    }

    // Assign density to particle
    particles[id].properties.y = density;

    // Determine pressure based on Tait's Equation (7) of WCSPH paper
    const float B = (cs * cs * p_0) / 7.0f;

    // Clamp pressure to prevent tensile instability artifacts!
    // Not present in original WCSPH paper but other work does mention this
    particles[id].properties.z = fmaxf(0.0f, B * (pow((density / p_0), 7) - 1.0f));
}

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
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, hashes, offsets, hasBoundary, sdfMap, volumeMap, domain, h, cs, p_0
    );
}

__global__ void sphForcesKernel(
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
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    float4 position = particle->position;

    // Get cell of particle
    int cellX = floor(position.x / h);
    int cellY = floor(position.y / h);
    int cellZ = floor(position.z / h);

    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

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

                    const ParticleGPU* other = &particles[neighborIndex];
                    const float4 diff = position - other->position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        // Pressure
                        particle->pressureForce +=
                            other->properties.x *
                            (
                                particle->properties.z / (particle->properties.y * particle->properties.y + 1e-6f) +
                                other->properties.z    / (other->properties.y * other->properties.y + 1e-6f)
                            ) *
                            spikyKernelGradient(diff, h);

                        // Viscosity
                        float4 vab = particle->velocity - other->velocity;

                        float dotTerm = dot(vab, diff);

                        if (dotTerm < 0) {
                            const float eps = 0.01f;

                            const float nu =
                                (2.0f * alpha * h * cs) /
                                (particle->properties.y + other->properties.y + 1e-6f);

                            const float pi_ab =
                                -nu * (dotTerm / (distSq + eps * h2));

                            particle->viscosityForce += other->properties.x * pi_ab * spikyKernelGradient(diff, h);
                        }

                        // Surface Tension
                        particle->surfaceTensionForce += other->properties.x * poly6Kernel(diff, h) * diff;
                    }
                }
            }
        }
    }

    particle->pressureForce = -1.0f * particle->pressureForce;
    particle->viscosityForce = -1.0f * particle->viscosityForce;
    particle->surfaceTensionForce = -1.0f * (tension / particle->properties.x) * particle->surfaceTensionForce;

    particle->flags.x =
        particle->surfaceTensionForce.x * particle->surfaceTensionForce.x +
        particle->surfaceTensionForce.y * particle->surfaceTensionForce.y +
        particle->surfaceTensionForce.z * particle->surfaceTensionForce.z;

    // if (surfaceTensionMagnitude2 > 2) {
    //     particle->flags.x = 1.0;
    // } else {
    //     particle->flags.x = 0.0;
    // }

    // Handle boundary force
    if (hasBoundary) {
        float3 boundaryCellSize = make_float3(
            (domain.max.x - domain.min.x) / volumeMap.resolution.x,
            (domain.max.y - domain.min.y) / volumeMap.resolution.y,
            (domain.max.z - domain.min.z) / volumeMap.resolution.z
        );

        // Determine boundary cell coords and clamp to valid range
        int boundaryCellX = max(0, min(static_cast<int>((position.x - domain.min.x) / boundaryCellSize.x), volumeMap.resolution.x - 1));
        int boundaryCellY = max(0, min(static_cast<int>((position.y - domain.min.y) / boundaryCellSize.y), volumeMap.resolution.y - 1));
        int boundaryCellZ = max(0, min(static_cast<int>((position.z - domain.min.z) / boundaryCellSize.z), volumeMap.resolution.z - 1));

        int boundaryCellId = boundaryCellX
            + boundaryCellY * volumeMap.resolution.x
            + boundaryCellZ * volumeMap.resolution.x * volumeMap.resolution.y;

        Hex32Cell* sdfCell      = &sdfMap.d_cells[boundaryCellId];
        Hex32Cell* volumeCell   = &volumeMap.d_cells[boundaryCellId];

        // Cell does not contribute any additional force if unused, meaning the cell is far away from any boundary
        if (volumeCell->used) {
            float3 boundaryCellMin = make_float3(
                domain.min.x + boundaryCellX * boundaryCellSize.x,
                domain.min.y + boundaryCellY * boundaryCellSize.y,
                domain.min.z + boundaryCellZ * boundaryCellSize.z
            );

            float3 boundaryCellMax = make_float3(
                boundaryCellMin.x + boundaryCellSize.x,
                boundaryCellMin.y + boundaryCellSize.y,
                boundaryCellMin.z + boundaryCellSize.z
            );

            float3 isoparametricCoords = T(boundaryCellMin, boundaryCellMax, make_float3(position.x, position.y, position.z));
            isoparametricCoords.x = fmaxf(-1.0f, fminf(1.0f, isoparametricCoords.x));
            isoparametricCoords.y = fmaxf(-1.0f, fminf(1.0f, isoparametricCoords.y));
            isoparametricCoords.z = fmaxf(-1.0f, fminf(1.0f, isoparametricCoords.z));

            float interpolatedSDF = interpolateHex32(
                isoparametricCoords.x,
                isoparametricCoords.y,
                isoparametricCoords.z,
                sdfCell->nodes
            );

            // Determine gradient of current position to calculate nearest point on sdf
            float3 gradLocal = interpolateHex32Gradient(
                isoparametricCoords.x, isoparametricCoords.y, isoparametricCoords.z,
                sdfCell->nodes
            );

            float3 gradSDF = make_float3(
                gradLocal.x * 2.0f / (boundaryCellMax.x - boundaryCellMin.x),
                gradLocal.y * 2.0f / (boundaryCellMax.y - boundaryCellMin.y),
                gradLocal.z * 2.0f / (boundaryCellMax.z - boundaryCellMin.z)
            );

            float len = sqrtf(gradSDF.x * gradSDF.x + gradSDF.y * gradSDF.y + gradSDF.z * gradSDF.z + 1e-10f);
            float3 normal = make_float3(gradSDF.x / len, gradSDF.y / len, gradSDF.z / len);

            float4 nearest = make_float4(
                position.x - normal.x * interpolatedSDF,
                position.y - normal.y * interpolatedSDF,
                position.z - normal.z * interpolatedSDF,
                0.0
            );

            float4 diff = make_float4(
                position.x - nearest.x,
                position.y - nearest.y,
                position.z - nearest.z,
                0.0
            );

            float interpolatedVolume = interpolateHex32(
                isoparametricCoords.x,
                isoparametricCoords.y,
                isoparametricCoords.z,
                volumeCell->nodes
            );

            particle->pressureForce -=
                interpolatedVolume * 1000.0f *
                (
                    (particle->properties.z / (particle->properties.y * particle->properties.y + 1e-6f)) +
                    (particle->properties.z / (1000.0f * 1000.0f))
                ) * spikyKernelGradient(diff, h);
        }
    }
}

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
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, hashes, offsets, hasBoundary, sdfMap, volumeMap, domain, h, alpha, cs, tension, p_0
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

struct AccelerationSq {
    __device__ float operator()(const ParticleGPU& p) const {
        float totalAccelerationX = p.pressureForce.x + p.viscosityForce.x + p.surfaceTensionForce.x;
        float totalAccelerationY = p.pressureForce.y + p.viscosityForce.y + p.surfaceTensionForce.y;
        float totalAccelerationZ = p.pressureForce.z + p.viscosityForce.z + p.surfaceTensionForce.z;

        return (totalAccelerationX * totalAccelerationX) + (totalAccelerationY * totalAccelerationY) + (totalAccelerationZ * totalAccelerationZ);
    }
};

float getMaxAcceleration(
    int numParticles,
    ParticleGPU* particles
) {
    const thrust::device_ptr<ParticleGPU> begin(particles);
    const thrust::device_ptr<ParticleGPU> end(particles + numParticles);

    const float maxAcc = thrust::transform_reduce(
        begin,
        end,
        AccelerationSq(),
        0.0f,
        thrust::maximum<float>()
    );

    return sqrtf(maxAcc);
}

__device__ float3 toLocal(float3 p, const CudaForceField& f) {
    float3 d = p - f.position;

    return make_float3(
        dot(d, f.right),
        dot(d, f.up),
        dot(d, f.forward)
    );
}

__device__ bool isInsideField(float3 worldPos, const CudaForceField& f) {
    float3 p = toLocal(worldPos, f);

    return (fabsf(p.x) <= f.halfExtents.x &&
            fabsf(p.y) <= f.halfExtents.y &&
            fabsf(p.z) <= f.halfExtents.z);
}

__device__ float4 toWorldForce(float4 fLocal, const CudaForceField& f) {
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
    const CudaForceField* forceFields,
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
    float4 acceleration = force + make_float4(0.0f, gravity, 0.0f, 0.0f);

    // Check force fields
    for (int f = 0 ; f < numForceFields; f++) {
        if (!forceFields[f].active)
            continue;

        if (isInsideField(make_float3(particle->position.x, particle->position.y, particle->position.z), forceFields[f])) {
            acceleration += toWorldForce({ forceFields[f].strength, 0.0, 0.0, 0.0 }, forceFields[f]);
        }
    }

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

    // TODO - Implement correctly
    // particle->velocity += (0.5f * deltaTime) * acceleration;
    // particle->position += deltaTime * particle->velocity;
    // particle->velocity += (0.5f * deltaTime) * acceleration;
}

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const CudaForceField* forceFields,
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
    rendererParticles[id].color = particles[id].color;
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
    CudaParticleSpawn spawn,
    ParticleGPU* particles,
    int* particleCount,
    const int3 gridCount
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= gridCount.x * gridCount.y * gridCount.z)
        return;

    // Map id to 3d position
    const int gx = id % gridCount.x;
    const int gy = (id / gridCount.x) % gridCount.y;
    const int gz = id / (gridCount.x * gridCount.y);

    float posX = spawn.area.min.x + gx * spawn.particleSpacing;
    float posY = spawn.area.min.y + gy * spawn.particleSpacing;
    float posZ = spawn.area.min.z + gz * spawn.particleSpacing;

    if (spawn.hasShape) {

    } else {
        int slot = atomicAdd(particleCount, 1);

        ParticleGPU* particle = &particles[slot];
        particle->position = { posX, posY, posZ, 0.0 };
        particle->startPosition = particle->position;
        particle->velocity = { 0.0, 0.0, 0.0, 0.0 };
        particle->properties = { spawn.restDensity, 0.0, 0.0, static_cast<float>(slot) };
        particle->flags = { 0.0, 0.0, 0.0, 0.0 };
        particle->color = spawn.color;
        particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
        particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
        particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

void launchInitParticlesKernel(
    CudaParticleSpawn spawn,
    ParticleGPU* particles,
    int* particleCount,
    const int3 gridCount
) {
    int threadsPerBlock = 256;
    int blocks = (gridCount.x * gridCount.y * gridCount.z + threadsPerBlock - 1) / threadsPerBlock;

    initParticlesKernel<<<blocks, threadsPerBlock>>>(
        spawn, particles, particleCount, gridCount
    );
}

__global__ void updateParticleMassKernel(
    int numParticles,
    ParticleGPU* particles,
    float mass
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];
    particle->properties.x = mass;
}

void launchUpdateParticleMassKernel(
    int numParticles,
    ParticleGPU* particles,
    float mass
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    updateParticleMassKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, mass
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
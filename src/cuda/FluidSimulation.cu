#include "FluidSimulation.h"
#include "cuda/Kernels.cuh"
#include "cuda/Math.cuh"
#include <cub/cub.cuh>
#include "Hex32.cuh"
#include "SDF.cuh"
#include <thrust/sort.h>
#include <cuda/CudaForceField.h>
#include "rendering/RendererParticle.h"
#include "simulation/VoxelField.h"

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

__global__ void hashParticlesKernel(
    const int numParticles,
    const float4* position,
    int* particleHashValue,
    int* particleIndex,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int cellX = floor(position[id].x / h);
    int cellY = floor(position[id].y / h);
    int cellZ = floor(position[id].z / h);

    uint hash = hashCell(cellX, cellY, cellZ, numParticles);

    particleHashValue[id] = hash;
    particleIndex[id] = id;
}

void launchHashParticlesKernel(
    const int numParticles,
    const float4* position,
    int* particleHashValue,
    int* particleIndex,
    const float h
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    hashParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, position, particleHashValue, particleIndex, h
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
    const ParticlesGPU particles,
    const int* particleIndices,
    ParticlesGPU sortedParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    if (id >= numParticles) return;

    // Move particle to its sorted index
    sortedParticles.d_position[id]              = particles.d_position[particleIndices[id]];
    sortedParticles.d_startPosition[id]         = particles.d_startPosition[particleIndices[id]];
    sortedParticles.d_velocity[id]              = particles.d_velocity[particleIndices[id]];
    sortedParticles.d_properties[id]            = particles.d_properties[particleIndices[id]];
    sortedParticles.d_flags[id]                 = particles.d_flags[particleIndices[id]];
    sortedParticles.d_color[id]                 = particles.d_color[particleIndices[id]];
    sortedParticles.d_pressureForce[id]         = particles.d_pressureForce[particleIndices[id]];
    sortedParticles.d_viscosityForce[id]        = particles.d_viscosityForce[particleIndices[id]];
    sortedParticles.d_surfaceTensionForce[id]   = particles.d_surfaceTensionForce[particleIndices[id]];
}

void launchReorderParticlesKernel(
    const int numParticles,
    const ParticlesGPU particles,
    const int* particleIndices,
    ParticlesGPU sortedParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    reorderParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndices, sortedParticles
    );
}

__global__ void densityPressureKernel(
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
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int cellX = floor(position[id].x / h);
    int cellY = floor(position[id].y / h);
    int cellZ = floor(position[id].z / h);

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

                    // const ParticleGPU other = particles[neighborIndex];
                    const float4 diff = position[id] - position[neighborIndex];

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        density += properties[neighborIndex].x * poly6Kernel(diff, h);
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
        int boundaryCellX = max(0, min(static_cast<int>((position[id].x - domain.min.x) / boundaryCellSize.x), volumeMap.resolution.x - 1));
        int boundaryCellY = max(0, min(static_cast<int>((position[id].y - domain.min.y) / boundaryCellSize.y), volumeMap.resolution.y - 1));
        int boundaryCellZ = max(0, min(static_cast<int>((position[id].z - domain.min.z) / boundaryCellSize.z), volumeMap.resolution.z - 1));

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

            float3 isoparametricCoords = T(boundaryCellMin, boundaryCellMax, make_float3(position[id].x, position[id].y, position[id].z));
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
    properties[id].y = density;

    // Determine pressure based on Tait's Equation (7) of WCSPH paper
    const float B = (cs * cs * p_0) / 7.0f;

    // Clamp pressure to prevent tensile instability artifacts!
    // Not present in original WCSPH paper but other work does mention this
    properties[id].z = fmaxf(0.0f, B * (pow((density / p_0), 7) - 1.0f));
}

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
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureKernel<<<blocks, threadsPerBlock>>>(
        numParticles, position, properties, hashes, offsets, hasBoundary, sdfMap, volumeMap, domain, h, cs, p_0
    );
}

__global__ void sphForcesKernel(
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
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int cellX = floor(position[id].x / h);
    int cellY = floor(position[id].y / h);
    int cellZ = floor(position[id].z / h);

    pressureForce[id]       = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    viscosityForce[id]      = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    surfaceTensionForce[id] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

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

                    const float4 diff = position[id] - position[neighborIndex];

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        // Pressure
                        pressureForce[id] +=
                            properties[neighborIndex].x *
                            (
                                properties[id].z / (properties[id].y * properties[id].y + 1e-6f) +
                                properties[neighborIndex].z    / (properties[neighborIndex].y * properties[neighborIndex].y + 1e-6f)
                            ) *
                            spikyKernelGradient(diff, h);

                        // Viscosity
                        float4 vab = velocity[id] - velocity[neighborIndex];

                        float dotTerm = dot(vab, diff);

                        if (dotTerm < 0) {
                            const float eps = 0.01f;

                            const float nu =
                                (2.0f * alpha * h * cs) /
                                (properties[id].y + properties[neighborIndex].y + 1e-6f);

                            const float pi_ab =
                                -nu * (dotTerm / (distSq + eps * h2));

                            viscosityForce[id] += properties[neighborIndex].x * pi_ab * spikyKernelGradient(diff, h);
                        }

                        // Surface Tension
                        surfaceTensionForce[id] += properties[neighborIndex].x * poly6Kernel(diff, h) * diff;
                    }
                }
            }
        }
    }

    pressureForce[id] = -1.0f * pressureForce[id];
    viscosityForce[id] = -1.0f * viscosityForce[id];
    surfaceTensionForce[id] = -1.0f * (tension / properties[id].x) * surfaceTensionForce[id];

    flags[id].x =
        surfaceTensionForce[id].x * surfaceTensionForce[id].x +
        surfaceTensionForce[id].y * surfaceTensionForce[id].y +
        surfaceTensionForce[id].z * surfaceTensionForce[id].z;

    // Handle boundary force
    if (hasBoundary) {
        float3 boundaryCellSize = make_float3(
            (domain.max.x - domain.min.x) / volumeMap.resolution.x,
            (domain.max.y - domain.min.y) / volumeMap.resolution.y,
            (domain.max.z - domain.min.z) / volumeMap.resolution.z
        );

        // Determine boundary cell coords and clamp to valid range
        int boundaryCellX = max(0, min(static_cast<int>((position[id].x - domain.min.x) / boundaryCellSize.x), volumeMap.resolution.x - 1));
        int boundaryCellY = max(0, min(static_cast<int>((position[id].y - domain.min.y) / boundaryCellSize.y), volumeMap.resolution.y - 1));
        int boundaryCellZ = max(0, min(static_cast<int>((position[id].z - domain.min.z) / boundaryCellSize.z), volumeMap.resolution.z - 1));

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

            float3 isoparametricCoords = T(boundaryCellMin, boundaryCellMax, make_float3(position[id].x, position[id].y, position[id].z));
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
                position[id].x - normal.x * interpolatedSDF,
                position[id].y - normal.y * interpolatedSDF,
                position[id].z - normal.z * interpolatedSDF,
                0.0
            );

            float4 diff = make_float4(
                position[id].x - nearest.x,
                position[id].y - nearest.y,
                position[id].z - nearest.z,
                0.0
            );

            float interpolatedVolume = interpolateHex32(
                isoparametricCoords.x,
                isoparametricCoords.y,
                isoparametricCoords.z,
                volumeCell->nodes
            );

            pressureForce[id] -=
                interpolatedVolume * p_0 *
                (
                    (properties[id].z / (properties[id].y * properties[id].y + 1e-6f)) +
                    (properties[id].z / (p_0 * p_0))
                ) * spikyKernelGradient(diff, h);
        }
    }
}

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
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, position, properties, velocity, pressureForce, viscosityForce, surfaceTensionForce, flags, hashes, offsets, hasBoundary, sdfMap, volumeMap, domain, h, alpha, cs, tension, p_0
    );
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
    const float4* pressureForce,
    const float4* viscosityForce,
    const float4* surfaceTensionForce,
    float4* velocity,
    float4* position,
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

    const float4 force = pressureForce[id] + viscosityForce[id] + surfaceTensionForce[id];
    float4 acceleration = force + make_float4(0.0f, gravity, 0.0f, 0.0f);

    // Check force fields
    for (int f = 0 ; f < numForceFields; f++) {
        if (!forceFields[f].active)
            continue;

        if (isInsideField(make_float3(position[id].x, position[id].y, position[id].z), forceFields[f])) {
            acceleration += toWorldForce({ forceFields[f].strength, 0.0, 0.0, 0.0 }, forceFields[f]);
        }
    }

    velocity[id] += deltaTime * acceleration;
    position[id] += deltaTime * velocity[id];

    // Domain boundaries
    if (position[id].x < domain.min.x) {
        position[id].x = domain.min.x;
        velocity[id].x *= -0.4f;
    }

    if (position[id].x > domain.max.x) {
        position[id].x = domain.max.x;
        velocity[id].x *= -0.4f;
    }

    if (position[id].y < domain.min.y) {
        position[id].y = domain.min.y;
        velocity[id].y *= -0.4f;
    }

    if (position[id].y > domain.max.y) {
        position[id].y = domain.max.y;
        velocity[id].y *= -0.4f;
    }

    if (position[id].z < domain.min.z) {
        position[id].z = domain.min.z;
        velocity[id].z *= -0.4f;
    }

    if (position[id].z > domain.max.z) {
        position[id].z = domain.max.z;
        velocity[id].z *= -0.4f;
    }
}

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
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    updateParticlesKernel<<<blocks, threadsPerBlock>>>(
        pressureForce, viscosityForce, surfaceTensionForce, velocity, position, numParticles, forceFields, numForceFields, deltaTime, domain, gravity
    );
}

__global__ void renderPrepareKernel(
    const int numParticles,
    const float4* position,
    const float4* velocity,
    const float4* properties,
    const float4* flags,
    const float4* color,
    RendererParticle* rendererParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    rendererParticles[id].position      = position[id];
    rendererParticles[id].velocity      = velocity[id];
    rendererParticles[id].properties    = properties[id];
    rendererParticles[id].flags         = flags[id];
    rendererParticles[id].color         = color[id];
}

void launchRenderPrepareKernel(
    const int numParticles,
    const float4* position,
    const float4* velocity,
    const float4* properties,
    const float4* flags,
    const float4* color,
    RendererParticle* rendererParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    renderPrepareKernel<<<blocks, threadsPerBlock>>>(
        numParticles, position, velocity, properties, flags, color, rendererParticles
    );
}

__global__ void initParticlesKernel(
    CudaParticleSpawn spawn,
    ParticlesGPU particles,
    int* particleCount,
    const CudaAABB domain,
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

    // Skip particles which would spawn outside the simulation domain
    if (    posX < domain.min.x || posY < domain.min.y || posZ < domain.min.z
         || posX > domain.max.x || posY > domain.max.y || posZ > domain.max.z ) {
        return;
    }

    if (spawn.hasShape && calculateSDF(make_float3(posX, posY, posZ), spawn.triangleCount, spawn.d_shapeTriangles) > 0.0)
        return;

    float mass = spawn.restDensity * spawn.particleSpacing * spawn.particleSpacing * spawn.particleSpacing;

    int slot = atomicAdd(particleCount, 1);

    particles.d_position[slot]              = make_float4(posX, posY, posZ, 0.0);
    particles.d_startPosition[slot]         = particles.d_position[slot];
    particles.d_velocity[slot]              = make_float4(0.0, 0.0, 0.0, 0.0);
    particles.d_properties[slot]            = make_float4(mass, 0.0, 0.0, static_cast<float>(slot));
    particles.d_flags[slot]                 = make_float4(0.0, 0.0, 0.0, 0.0);
    particles.d_color[slot]                 = spawn.color;
    particles.d_pressureForce[slot]         = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particles.d_viscosityForce[slot]        = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particles.d_surfaceTensionForce[slot]   = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void launchInitParticlesKernel(
    CudaParticleSpawn spawn,
    ParticlesGPU particles,
    int* particleCount,
    const CudaAABB domain,
    const int3 gridCount
) {
    int threadsPerBlock = 256;
    int blocks = (gridCount.x * gridCount.y * gridCount.z + threadsPerBlock - 1) / threadsPerBlock;

    initParticlesKernel<<<blocks, threadsPerBlock>>>(
        spawn, particles, particleCount, domain, gridCount
    );
}

__global__ void resetParticlesKernel(
    float4* position,
    const float4* startPosition,
    float4* velocity,
    float4* properties,
    float4* pressureForce,
    float4* viscosityForce,
    float4* surfaceTensionForce,
    const int numParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    position[id] = startPosition[id];
    velocity[id] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    properties[id].y = 0.0f;
    properties[id].z = 0.0f;
    pressureForce[id] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    viscosityForce[id] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    surfaceTensionForce[id] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void launchResetParticlesKernel(
    float4* position,
    const float4* startPosition,
    float4* velocity,
    float4* properties,
    float4* pressureForce,
    float4* viscosityForce,
    float4* surfaceTensionForce,
    const int numParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    resetParticlesKernel<<<blocks, threadsPerBlock>>>(
        position, startPosition, velocity, properties, pressureForce, viscosityForce, surfaceTensionForce, numParticles
    );
}

__global__ void topSurfaceParticlesKernel(
    const int numParticles,
    const float4* position,
    float4* flags,
    const int* hashes,
    const int* offsets,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int cellX = floor(position[id].x / h);
    int cellY = floor(position[id].y / h);
    int cellZ = floor(position[id].z / h);

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

                    // Skip self
                    if (neighborIndex == id)
                        continue;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    const float4 diff = position[id] - position[neighborIndex];

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    float horizontal = sqrtf(diff.x * diff.x + diff.z * diff.z);

                    if (distSq < h2 && diff.y < -0.15f * h && horizontal < -diff.y) {
                        flags[id].y = 0.0f;
                        return;
                    }
                }
            }
        }
    }

    flags[id].y = 1.0f;
}

void launchTopSurfaceParticlesKernel(
    const int numParticles,
    const float4* position,
    float4* flags,
    const int* hashes,
    const int* offsets,
    const float h
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    topSurfaceParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, position, flags, hashes, offsets, h
    );
}

__global__ void voxelFieldKernel(
    const VoxelField field,
    const CudaAABB domain,
    const int numParticles,
    const float4* position,
    const float4* properties,
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

                    const float4 diff = samplePosition - position[neighborIndex];

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        float invDensity = 1.0f / (properties[neighborIndex].y + 1e-6f);

                        // Color field
                        n += properties[neighborIndex].x * invDensity * poly6KernelGradient(diff, h);
                        c += properties[neighborIndex].x * invDensity * poly6Kernel(diff, h);
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
    const float4* position,
    const float4* properties,
    const int* hashes,
    const int* offsets,
    const float h
) {
    int threadsPerBlock = 256;
    int cells = field.resolution.x * field.resolution.y * field.resolution.z;
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    voxelFieldKernel<<<blocks, threadsPerBlock>>>(
        field, domain, numParticles, position, properties, hashes, offsets, h
    );
}
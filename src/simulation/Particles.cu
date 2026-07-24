#include "simulation/Particles.cuh"

#include <cstdio>

#include "../../include/simulation/scene/ForceField.h"
#include "rendering/RendererParticle.h"
#include "simulation/AABB.cuh"
#include "simulation/Kernels.cuh"
#include "simulation/ParticlesGPU.h"
#include "simulation/boundaries/Hex32Field.h"
#include "simulation/scene/IParticleSpawn.h"
#include "simulation/utils/Cell.cuh"
#include "simulation/utils/Hex32.cuh"
#include "simulation/utils/SDF.cuh"

__global__ void initParticlesKernel(
    ParticleSpawn spawn,
    ParticlesGPU particles,
    int* particleCount,
    const AABBView domain,
    const glm::ivec3 gridCount
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= gridCount.x * gridCount.y * gridCount.z)
        return;

    // Map id to 3d position
    const int gx = id % gridCount.x;
    const int gy = (id / gridCount.x) % gridCount.y;
    const int gz = id / (gridCount.x * gridCount.y);

    glm::vec3 pos = spawn.area.min + glm::vec3(gx, gy, gz) * spawn.particleSpacing;

    // Skip particles which would spawn outside the simulation domain
    if ( pos.x < domain.min.x || pos.y < domain.min.y || pos.z < domain.min.z
      || pos.x > domain.max.x || pos.y > domain.max.y || pos.z > domain.max.z ) {
        return;
    }


    if (spawn.hasShape && calculateSDF(pos, spawn.triangleCount, spawn.d_shapeTriangles) > 0.0)
        return;


    float mass = spawn.restDensity * spawn.particleSpacing * spawn.particleSpacing * spawn.particleSpacing;

    int slot = atomicAdd(particleCount, 1);

    particles.d_position[slot]              = pos;
    particles.d_startPosition[slot]         = particles.d_position[slot];
    particles.d_velocity[slot]              = glm::vec3(0.0);
    particles.d_pressureForce[slot]         = glm::vec3(0.0);
    particles.d_viscosityForce[slot]        = glm::vec3(0.0);
    particles.d_surfaceTensionForce[slot]   = glm::vec3(0.0);
    particles.d_properties[slot]            = glm::vec4(mass, 0.0, 0.0, static_cast<float>(slot));
    particles.d_flags[slot]                 = glm::vec4(false);
    particles.d_color[slot]                 = spawn.color;
}

__global__ void resetParticlesKernel(
    glm::vec3* position,
    const glm::vec3* startPosition,
    glm::vec3* velocity,
    glm::vec4* properties,
    glm::vec3* pressureForce,
    glm::vec3* viscosityForce,
    glm::vec3* surfaceTensionForce,
    const int numParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    position[id] = startPosition[id];
    velocity[id] = glm::vec3(0.0f);
    properties[id].y = 0.0f;
    properties[id].z = 0.0f;
    pressureForce[id] = glm::vec3(0.0f);
    viscosityForce[id] = glm::vec3(0.0f);
    surfaceTensionForce[id] = glm::vec3(0.0f);
}

__global__ void renderPrepareKernel(
    const int numParticles,
    const glm::vec3* position,
    const glm::vec3* velocity,
    const glm::vec4* properties,
    const glm::vec4* flags,
    const glm::vec3* color,
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

__global__ void hashParticlesKernel(
    const int numParticles,
    const glm::vec3* position,
    int* particleHashValue,
    int* particleIndex,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    glm::ivec3 cell = glm::floor(position[id] / h);
    uint hash = hashCell(cell, numParticles);

    particleHashValue[id] = hash;
    particleIndex[id] = id;
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

__global__ void densityPressureKernel(
    const int numParticles,
    const glm::vec3* position,
    glm::vec4* properties,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field sdfMap,
    const Hex32Field volumeMap,
    const AABBView domain,
    const float h,
    const float cs,
    const float p_0
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    glm::ivec3 cell = glm::floor(position[id] / h);

    float density = 0.0f;
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cell + glm::ivec3(dx, dy, dz), numParticles);
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
                    const glm::vec3 diff = position[id] - position[neighborIndex];

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
        glm::vec3 boundaryCellSize = domain.size / static_cast<glm::vec3>(volumeMap.resolution);

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
            glm::vec3 boundaryCellMin = domain.min + glm::vec3(boundaryCellX, boundaryCellY, boundaryCellZ) * boundaryCellSize;
            glm::vec3 boundaryCellMax = boundaryCellMin + boundaryCellSize;

            glm::vec3 isoparametricCoords = T(boundaryCellMin, boundaryCellMax, position[id]);
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

__global__ void sphForcesKernel(
    const int numParticles,
    const glm::vec3* position,
    const glm::vec4* properties,
    const glm::vec3* velocity,
    glm::vec3* pressureForce,
    glm::vec3* viscosityForce,
    glm::vec3* surfaceTensionForce,
    glm::vec4* flags,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field sdfMap,
    const Hex32Field volumeMap,
    const AABBView domain,
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
    glm::ivec3 cell = glm::floor(position[id] / h);

    pressureForce[id]       = glm::vec3(0.0f);
    viscosityForce[id]      = glm::vec3(0.0f);
    surfaceTensionForce[id] = glm::vec3(0.0f);

    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cell + glm::ivec3(dx, dy, dz), numParticles);
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

                    const glm::vec3 diff = position[id] - position[neighborIndex];

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
                        glm::vec3 vab = velocity[id] - velocity[neighborIndex];

                        float dotTerm = glm::dot(vab, diff);

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
        glm::vec3 boundaryCellSize = domain.size / static_cast<glm::vec3>(volumeMap.resolution);

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
            glm::vec3 boundaryCellMin = domain.min + glm::vec3(boundaryCellX, boundaryCellY, boundaryCellZ) * boundaryCellSize;
            glm::vec3 boundaryCellMax = boundaryCellMin + boundaryCellSize;

            glm::vec3 isoparametricCoords = T(boundaryCellMin, boundaryCellMax, position[id]);
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
            glm::vec3 gradLocal = interpolateHex32Gradient(
                isoparametricCoords.x, isoparametricCoords.y, isoparametricCoords.z,
                sdfCell->nodes
            );

            glm::vec3 gradSDF = gradLocal * 2.0f / (boundaryCellMax - boundaryCellMin);
            glm::vec3 normal = glm::normalize(gradSDF);
            glm::vec3 nearest = position[id] - normal * interpolatedSDF;
            glm::vec3 diff = position[id] - nearest;

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

__device__ glm::vec3 toLocal(glm::vec3 p, const ForceField& f) {
    glm::vec3 d = p - f.position;

    return glm::vec3(
        dot(d, f.right),
        dot(d, f.up),
        dot(d, f.forward)
    );
}

__device__ bool isInsideField(glm::vec3 worldPos, const ForceField& f) {
    glm::vec3 p = toLocal(worldPos, f);

    return (fabsf(p.x) <= f.halfExtents.x &&
            fabsf(p.y) <= f.halfExtents.y &&
            fabsf(p.z) <= f.halfExtents.z);
}

__device__  glm::vec3 toWorldForce(glm::vec3 fLocal, const ForceField& f) {
    return glm::vec3(
        fLocal.x * f.right.x   + fLocal.y * f.up.x   + fLocal.z * f.forward.x,
        fLocal.x * f.right.y   + fLocal.y * f.up.y   + fLocal.z * f.forward.y,
        fLocal.x * f.right.z   + fLocal.y * f.up.z   + fLocal.z * f.forward.z
    );
}

__global__ void updateParticlesKernel(
    const glm::vec3* pressureForce,
    const glm::vec3* viscosityForce,
    const glm::vec3* surfaceTensionForce,
    glm::vec3* velocity,
    glm::vec3* position,
    const int numParticles,
    const ForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const AABBView domain,
    const float gravity
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    const glm::vec3 force = pressureForce[id] + viscosityForce[id] + surfaceTensionForce[id];
    glm::vec3 acceleration = force + glm::vec3(0.0f, gravity, 0.0f);

    // Check force fields
    for (int f = 0 ; f < numForceFields; f++) {
        if (!forceFields[f].active)
            continue;

        if (isInsideField(position[id], forceFields[f])) {
            acceleration += toWorldForce({ forceFields[f].strength, 0.0, 0.0 }, forceFields[f]);
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

__global__ void topSurfaceParticlesKernel(
    const int numParticles,
    const glm::vec3* position,
    glm::vec4* flags,
    const int* hashes,
    const int* offsets,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    glm::ivec3 cell = glm::floor(position[id] / h);

    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cell + glm::ivec3(dx, dy, dz), numParticles);
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

                    const glm::vec3 diff = position[id] - position[neighborIndex];

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    float horizontal = sqrtf(diff.x * diff.x + diff.z * diff.z);

                    if (distSq < h2 && diff.y < -0.15f * h && horizontal < -diff.y) {
                        flags[id].y = 0.0;
                        return;
                    }
                }
            }
        }
    }

    flags[id].y = 1.0;
}
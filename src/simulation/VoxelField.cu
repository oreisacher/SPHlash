//
// Created by ruut on 7/23/26.
//

#include "simulation/VoxelField.cuh"
#include "simulation/VoxelField.h"

#include <c++/16/cstdio>

#include "glm/common.hpp"
#include "glm/detail/func_geometric.inl"
#include "simulation/AABB.cuh"
#include "simulation/Kernels.cuh"
#include "simulation/utils/Cell.cuh"

__global__ void voxelFieldKernel(
    const VoxelField field,
    const AABBView domain,
    const int numParticles,
    const glm::vec3* position,
    const glm::vec4* properties,
    const int* hashes,
    int tableSize,
    const int* offsets,
    const float h,
    const KernelSettings kernelSettings
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = field.resolution.x * field.resolution.y * field.resolution.z;

    if (id >= cellCount)
        return;

    // Convert index to 3d cell pos
    int x = id % field.resolution.x;
    int y = (id / field.resolution.x) % field.resolution.y;
    int z = id / (field.resolution.x * field.resolution.y);

    glm::vec3 cellSizes = domain.size / static_cast<glm::vec3>(field.resolution - 1);
    glm::vec3 samplePosition = glm::vec3(x, y, z) * cellSizes + domain.min;

    const float supportRadius = h * getMaxSupportRadius(kernelSettings);
    const float supportRadius2 = supportRadius * supportRadius;

    // Get cell of sample point
    glm::ivec3 cell = glm::floor(samplePosition / supportRadius);

    glm::vec3 n(0.0f);
    float c = 0.0f;
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cell + glm::ivec3(dx, dy, dz), tableSize);
                int offset = offsets[hash];

                if (offset == -1)
                    continue;

                while (offset < numParticles) {
                    int neighborIndex = offset;
                    offset++;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    const glm::vec3 diff = samplePosition - position[neighborIndex];

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < supportRadius2) {
                        float invDensity = 1.0f / (properties[neighborIndex].y + 1e-6f);

                        // Color field
                        n += properties[neighborIndex].x * invDensity * evaluateKernelGradient(kernelSettings.density, diff, h);
                        c += properties[neighborIndex].x * invDensity * evaluateKernel(kernelSettings.density, sqrtf(distSq), h);
                    }
                }
            }
        }
    }

    field.d_color[id] = c - 0.5f;
    field.d_normal[id] = glm::vec3(-n.x, -n.y, -n.z);
}
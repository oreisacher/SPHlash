//
// Created by ruut on 7/23/26.
//

#include "simulation/VoxelField.cuh"
#include "simulation/VoxelField.h"
#include "cuda/CudaAABB.h"
#include "simulation/Kernels.cuh"
#include "simulation/utils/Cell.cuh"

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
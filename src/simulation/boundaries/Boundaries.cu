#include "simulation/boundaries/Boundaries.cuh"
#include "simulation/utils/Hex32.cuh"
#include "simulation/utils/SDF.cuh"
#include "simulation/boundaries/Hex32Field.h"
#include "../../cuda/CudaAABB.h"

__global__ void sdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    const Hex32Field hexField,
    const CudaAABB domain,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = hexField.resolution.x * hexField.resolution.y * hexField.resolution.z;

    if (id >= cellCount)
        return;

    int cellX = id % hexField.resolution.x;
    int cellY = (id / hexField.resolution.x) % hexField.resolution.y;
    int cellZ = id / (hexField.resolution.x * hexField.resolution.y);

    float3 cellSize = make_float3(
        (domain.max.x - domain.min.x) / hexField.resolution.x,
        (domain.max.y - domain.min.y) / hexField.resolution.y,
        (domain.max.z - domain.min.z) / hexField.resolution.z
    );

    float3 cellMinPos = make_float3(
        domain.min.x + cellX * cellSize.x,
        domain.min.y + cellY * cellSize.y,
        domain.min.z + cellZ * cellSize.z
    );

    Hex32Cell* cell = &hexField.d_cells[id];

    // Calculate sdf value for each node of cell
    for (int n = 0 ; n < 32 ; n++) {
        const Hex32Node& node = Hex32CubeLookup[n];

        float3 worldPos = make_float3(
            cellMinPos.x + node.x * cellSize.x,
            cellMinPos.y + node.y * cellSize.y,
            cellMinPos.z + node.z * cellSize.z
        );

        cell->nodes[n] = calculateSDF(worldPos, triangleCount, triangles);
    }

    // Determine if this cell is unnecessary
    float phiMin = INFINITY;
    float phiMax = -INFINITY;

    for (const float& value : cell->nodes) {
        if (value < phiMin)
            phiMin = value;

        if (value > phiMax)
            phiMax = value;
    }

    cell->used = (2.0f * h > phiMin && phiMax > -h);
}

__device__ float cubicSplineKernel(float x, float h) {
    float q = x / h;

    if (q >= 2.0f)
        return 0.0f;

    float alpha = 1.0f / (3.14159265358979323846f * h * h * h);

    if (q < 1.0f)
        return alpha * (1.0f - 1.5f * q * q + 0.75f * q * q * q);

    float t = (2.0f - q);
    return alpha * 0.25f * t * t * t;
}

__device__ float gammaStar(float x, float r) {
    if (x <= 0.0f)
        return 1.0f;

    if (x >= r)
        return 0.0f;

    return cubicSplineKernel(x, 0.5 * r) / cubicSplineKernel(0, 0.5 * r);
}

__global__ void volumeMapKernel(
    const Hex32Field sdfField,
    const Hex32Field volumeField,
    const CudaAABB domain,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = sdfField.resolution.x * sdfField.resolution.y * sdfField.resolution.z;

    if (id >= cellCount)
        return;

    Hex32Cell* sdfCell = &sdfField.d_cells[id];
    volumeField.d_cells[id].used = sdfCell->used;

    // Skip further calculations is cell is unused
    if (!sdfCell->used) {
        // Fill unused nodes with zeros
        for (auto& value : volumeField.d_cells[id].nodes)
            value = 0;

        return;
    }

    int cellX = id % sdfField.resolution.x;
    int cellY = (id / sdfField.resolution.x) % sdfField.resolution.y;
    int cellZ = id / (sdfField.resolution.x * sdfField.resolution.y);

    float3 cellSize = make_float3(
        (domain.max.x - domain.min.x) / sdfField.resolution.x,
        (domain.max.y - domain.min.y) / sdfField.resolution.y,
        (domain.max.z - domain.min.z) / sdfField.resolution.z
    );

    float3 cellMinPos = make_float3(
        domain.min.x + cellX * cellSize.x,
        domain.min.y + cellY * cellSize.y,
        domain.min.z + cellZ * cellSize.z
    );

    // Gauss-Legendre
    const float xi[5] = {
        (1.0f / 3.0f) * sqrt(5.0f + 2.0f * sqrt(10.0f / 7.0f)),
        (1.0f / 3.0f) * sqrt(5.0f - 2.0f * sqrt(10.0f / 7.0f)),
        0.0f,
        -1.0f * (1.0f / 3.0f) * sqrt(5.0f - 2.0f * sqrt(10.0f / 7.0f)),
        -1.0f * (1.0f / 3.0f) * sqrt(5.0f + 2.0f * sqrt(10.0f / 7.0f)),
    };

    const float w[5]  = {
        (322.0f - 13.0f * sqrt(70.0f)) / 900.0f,
        (322.0f + 13.0f * sqrt(70.0f)) / 900.0f,
        128.0f / 225.0f,
        (322.0f + 13.0f * sqrt(70.0f)) / 900.0f,
        (322.0f - 13.0f * sqrt(70.0f)) / 900.0f
    };

    for (int nodeIndex = 0 ; nodeIndex < 32 ; nodeIndex++) {
        Hex32Node node = Hex32CubeLookup[nodeIndex];

        float3 nodePos = make_float3(
            cellMinPos.x + node.x * cellSize.x,
            cellMinPos.y + node.y * cellSize.y,
            cellMinPos.z + node.z * cellSize.z
        );

        float volume = 0.0f;

        for (int z = 0 ; z < 5 ; z++) {
            for (int y = 0 ; y < 5 ; y++) {
                for (int x = 0 ; x < 5 ; x++) {
                    float3 queryPosition = make_float3(
                        nodePos.x + h * xi[x],
                        nodePos.y + h * xi[y],
                        nodePos.z + h * xi[z]
                    );

                    // Reject samples outside domain
                    if (queryPosition.x < domain.min.x || queryPosition.x > domain.max.x ||
                        queryPosition.y < domain.min.y || queryPosition.y > domain.max.y ||
                        queryPosition.z < domain.min.z || queryPosition.z > domain.max.z) {
                        continue;
                    }

                    // Find cell which contains sample position
                    int sampleCellX = min(max(static_cast<int>((queryPosition.x - domain.min.x) / cellSize.x), 0), sdfField.resolution.x - 1);
                    int sampleCellY = min(max(static_cast<int>((queryPosition.y - domain.min.y) / cellSize.y), 0), sdfField.resolution.y - 1);
                    int sampleCellZ = min(max(static_cast<int>((queryPosition.z - domain.min.z) / cellSize.z), 0), sdfField.resolution.z - 1);

                    // Get cell
                    int cellId = sampleCellX
                             + sampleCellY * sdfField.resolution.x
                             + sampleCellZ * sdfField.resolution.x * sdfField.resolution.y;
                    Hex32Cell* sampleCell = &sdfField.d_cells[cellId];

                    float3 sampleCellMin = make_float3(
                        domain.min.x + sampleCellX * cellSize.x,
                        domain.min.y + sampleCellY * cellSize.y,
                        domain.min.z + sampleCellZ * cellSize.z
                    );

                    float3 sampleCellMax = make_float3(
                        sampleCellMin.x + cellSize.x,
                        sampleCellMin.y + cellSize.y,
                        sampleCellMin.z + cellSize.z
                    );

                    // Interpolate sdf values of sample cell based on sample position
                    float3 isoparametricCoords = T(sampleCellMin, sampleCellMax, queryPosition);
                    float interpolatedSDF = interpolateHex32(
                        isoparametricCoords.x,
                        isoparametricCoords.y,
                        isoparametricCoords.z,
                        sampleCell->nodes
                    );

                    float gamma = gammaStar(interpolatedSDF, h);
                    volume += gamma * w[x] * w[y] * w[z];
                }
            }
        }

        volumeField.d_cells[id].nodes[nodeIndex] = volume * h * h * h;
    }
}
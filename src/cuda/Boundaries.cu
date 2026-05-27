#pragma once

#include "Boundaries.h"
#include "Math.cuh"
#include "utils/AABB.h"
#include "utils/Hex32Field.h"
#include "Kernels.cuh"
#include "curand_kernel.h"
#include "Hex32.cuh"

__device__ float distanceTriangle(float3 p, float3 v1, float3 v2, float3 v3) {
    float3 v21 = v2 - v1;
    float3 v32 = v3 - v2;
    float3 v13 = v1 - v3;

    float3 p1 = p - v1;
    float3 p2 = p - v2;
    float3 p3 = p - v3;

    float3 normal = cross(v21, v13);

    if (signf(dot(cross(v21, normal), p1)) +
        signf(dot(cross(v32, normal), p2)) +
        signf(dot(cross(v13, normal), p3)) < 2.0) {
        return sqrt(fminf(fminf(
            dot2(v21 * clampf(dot(v21, p1) / dot2(v21), 0.0, 1.0) - p1),
            dot2(v32 * clampf(dot(v32, p2) / dot2(v32), 0.0, 1.0) - p2)),
            dot2(v13 * clampf(dot(v13, p3) / dot2(v13), 0.0, 1.0) - p3)));
    }

    return sqrt(dot(normal, p1) * dot(normal, p1) / dot2(normal));
}

__device__ float solidAngle(float3 position, float3 v0, float3 v1, float3 v2) {
    float3 a = normalize(v0 - position);
    float3 b = normalize(v1 - position);
    float3 c = normalize(v2 - position);

    float numerator = dot(a, cross(b, c));

    float denom =
        1.0f +
        dot(a, b) +
        dot(b, c) +
        dot(c, a);

    return 2.0f * atan2f(numerator, denom);
}

__device__ float computeWindingNumber(float3 position, int triangleCount, const Triangle* triangles) {
    float winding = 0.0f;

    for (int i = 0; i < triangleCount; i++)
        winding += solidAngle(position, triangles[i].v0, triangles[i].v1, triangles[i].v2);

    return winding;
}

__device__ float calculateSDF(float3 position, int triangleCount, const Triangle* triangles) {
    float best = INFINITY;

    // Calculate distance to nearest triangle
    for (int i = 0 ; i < triangleCount ; i++) {
        float dist = distanceTriangle(position, triangles[i].v0, triangles[i].v1, triangles[i].v2);

        if (dist < best)
            best = dist;
    }

    // Determine sign of distance = Check if position is inside or outside Mesh
    float winding = computeWindingNumber(position, triangleCount, triangles);
    return fabsf(winding) > 3.1415926f ? -best : best;
}

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

    cell->used = (phiMax < 2.0f * h);
}

void launchSdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    const Hex32Field& hexField,
    const CudaAABB& domain,
    const float h
) {
    int threadsPerBlock = 256;
    int cells = hexField.resolution.x * hexField.resolution.y * hexField.resolution.z;
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    sdfKernel<<<blocks, threadsPerBlock>>>(
        triangleCount,
        triangles,
        hexField,
        domain,
        h
    );
}

/*
    Monotonically decreasing function which yields higher values for higher penetration depth.
    See Equation (8) in corresponding Paper.
*/
__device__ float gamma(float restDensity, float h, float x) {
    if (x <= h)
        return restDensity * (1 - x / h);
    return 0;
}

__device__ float3 sampleSphere(curandState* state, float3 center, float radius) {
    float x = curand_normal(state);
    float y = curand_normal(state);
    float z = curand_normal(state);

    float3 dir = normalize(make_float3(x, y, z));

    float r = cbrtf(curand_uniform(state)) * radius;

    return make_float3(
        center.x + dir.x * r,
        center.y + dir.y * r,
        center.z + dir.z * r
    );
}

__global__ void densityMapKernel(
    curandState* states,
    const Hex32Field sdfField,
    const Hex32Field densityField,
    const CudaAABB domain,
    float mcSamples,
    float restDensity,
    float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = sdfField.resolution.x * sdfField.resolution.y * sdfField.resolution.z;

    if (id >= cellCount)
        return;

    Hex32Cell* sdfCell = &sdfField.d_cells[id];
    densityField.d_cells[id].used = sdfCell->used;

    // Skip further calculations is cell is unused
    if (!sdfCell->used) {
        // Fill unused nodes with zeros
        for (auto& value : densityField.d_cells[id].nodes)
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

    // Estimate density for each node of cell
    for (int nodeIndex = 0 ; nodeIndex < 32 ; nodeIndex++) {
        Hex32Node node = Hex32CubeLookup[nodeIndex];

        float3 nodePos = make_float3(
            cellMinPos.x + node.x * cellSize.x,
            cellMinPos.y + node.y * cellSize.y,
            cellMinPos.z + node.z * cellSize.z
        );

        float density = 0;

        // Monte Carlo estimate for Equation (6) of the corresponding paper
        for (int sample = 0 ; sample < mcSamples ; sample++) {
            float3 samplePosition = sampleSphere(&states[id], nodePos, h);
            float3 diff = nodePos - samplePosition;

            // Reject samples outside domain
            if (samplePosition.x < domain.min.x || samplePosition.x > domain.max.x ||
                samplePosition.y < domain.min.y || samplePosition.y > domain.max.y ||
                samplePosition.z < domain.min.z || samplePosition.z > domain.max.z) {
                continue;
            }

            // Find cell which contains sample position
            int sampleCellX = static_cast<int>((samplePosition.x - domain.min.x) / cellSize.x);
            int sampleCellY = static_cast<int>((samplePosition.y - domain.min.y) / cellSize.y);
            int sampleCellZ = static_cast<int>((samplePosition.z - domain.min.z) / cellSize.z);

            // Get cell
            int cellId = sampleCellX
                     + sampleCellY * sdfField.resolution.x
                     + sampleCellZ * sdfField.resolution.x * sdfField.resolution.y;
            Hex32Cell* sampleCell = &sdfField.d_cells[cellId];

            // Unsued cell -> Does not contribute anything to density
            if (sampleCell->used == false)
                continue;

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
            float3 isoparametricCoords = T(sampleCellMin, sampleCellMax, samplePosition);
            float interpolatedSDF = interpolateHex32(
                isoparametricCoords.x,
                isoparametricCoords.y,
                isoparametricCoords.z,
                sampleCell->nodes
            );

            float g = gamma(restDensity, h, interpolatedSDF);
            float w = poly6Kernel(make_float4(diff.x, diff.y, diff.z, 0.0f), h);
            density += g * w;
        }

        float V = (4.0 / 3.0f) * M_PI * h * h * h;
        densityField.d_cells[id].nodes[nodeIndex] = density * (V / mcSamples);
    }
}

__global__ void initCuRand(curandState* states, unsigned long long seed, int n) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= n)
        return;

    curand_init(seed, id, 0, &states[id]);
}

void launchDensityMapKernel(
    const Hex32Field sdfField,
    const Hex32Field densityField,
    const CudaAABB domain,
    float mcSamples,
    float restDensity,
    float h
) {
    int threadsPerBlock = 256;
    int cells = sdfField.resolution.x * sdfField.resolution.y * sdfField.resolution.z;
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    curandState* d_states;
    cudaMalloc(&d_states, cells * sizeof(curandState));

    // Init random states of curand
    initCuRand<<<blocks, threadsPerBlock>>>(d_states, 0xDEADBEEF, cells);

    densityMapKernel<<<blocks, threadsPerBlock>>>(
        d_states,
        sdfField,
        densityField,
        domain,
        mcSamples,
        restDensity,
        h
    );

    cudaFree(d_states);
}
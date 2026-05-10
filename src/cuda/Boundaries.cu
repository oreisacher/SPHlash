#include "Boundaries.h"

#include <sys/stat.h>

#include "Math.cuh"
#include "utils/AABB.h"
#include "utils/Hex32Field.h"
#include "Kernels.cuh"
#include "curand_kernel.h"

#define IDX(x,y,z) ((x) + (y) * resolution + (z) * resolution * resolution)

struct HexNode {
    float xi, eta, zeta;
    bool isCorner;
};

__constant__ HexNode hex32Nodes[32] = {
    // Corner nodes
    { -1.0f, -1.0f, -1.0f, true },
    {  1.0f, -1.0f, -1.0f, true },
    {  1.0f,  1.0f, -1.0f, true },
    { -1.0f,  1.0f, -1.0f, true },
    { -1.0f, -1.0f,  1.0f, true },
    {  1.0f, -1.0f,  1.0f, true },
    {  1.0f,  1.0f,  1.0f, true },
    { -1.0f,  1.0f,  1.0f, true },

    // Edge nodes along xi
    { -1.0f/3.0f, -1.0f, -1.0f, false },
    {  1.0f/3.0f, -1.0f, -1.0f, false },
    { -1.0f/3.0f,  1.0f, -1.0f, false },
    {  1.0f/3.0f,  1.0f, -1.0f, false },
    { -1.0f/3.0f, -1.0f,  1.0f, false },
    {  1.0f/3.0f, -1.0f,  1.0f, false },
    { -1.0f/3.0f,  1.0f,  1.0f, false },
    {  1.0f/3.0f,  1.0f,  1.0f, false },

    // Edge nodes along eta
    { -1.0f, -1.0/3.0f, -1.0f, false },
    {  1.0f, -1.0/3.0f, -1.0f, false },
    { -1.0f,  1.0/3.0f, -1.0f, false },
    {  1.0f,  1.0/3.0f, -1.0f, false },
    { -1.0f, -1.0/3.0f,  1.0f, false },
    {  1.0f, -1.0/3.0f,  1.0f, false },
    { -1.0f,  1.0/3.0f,  1.0f, false },
    {  1.0f,  1.0/3.0f,  1.0f, false },

    // Edge nodes along zeta
    { -1.0f, -1.0f, -1.0/3.0f, false },
    {  1.0f, -1.0f, -1.0/3.0f, false },
    { -1.0f,  1.0f, -1.0/3.0f, false },
    {  1.0f,  1.0f, -1.0/3.0f, false },
    { -1.0f, -1.0f,  1.0/3.0f, false },
    {  1.0f, -1.0f,  1.0/3.0f, false },
    { -1.0f,  1.0f,  1.0/3.0f, false },
    {  1.0f,  1.0f,  1.0/3.0f, false },
};

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

__device__ float3 normalize3(float3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    if (len > 1e-8f)
    {
        float invLen = 1.0f / len;
        return make_float3(v.x * invLen, v.y * invLen, v.z * invLen);
    }

    return make_float3(0.0f, 0.0f, 0.0f);
}

__device__ float solidAngle(float3 p, float3 v0, float3 v1, float3 v2)
{
    // Vectors from point to triangle vertices
    float3 a = v0 - p;
    float3 b = v1 - p;
    float3 c = v2 - p;

    // Normalize
    a = normalize3(a);
    b = normalize3(b);
    c = normalize3(c);

    // Robust solid angle computation
    float numerator = dot(a, cross(b, c));

    float denom =
        1.0f +
        dot(a, b) +
        dot(b, c) +
        dot(c, a);

    return 2.0f * atan2f(numerator, denom);
}

__device__ float computeWindingNumber(
    float3 p,
    int triangleCount,
    const Triangle* triangles)
{
    float winding = 0.0f;

    for (int i = 0; i < triangleCount; i++)
    {
        winding += solidAngle(
            p,
            triangles[i].v0,
            triangles[i].v1,
            triangles[i].v2
        );
    }

    return winding;
}

__device__ float calculateSDF(float3 position, int triangleCount, const Triangle* triangles) {
    float best = 1e20;

    for (int i = 0 ; i < triangleCount ; i++) {
        float dist = distanceTriangle(position, triangles[i].v0, triangles[i].v1, triangles[i].v2);

        if (dist < best)
            best = dist;
    }

    float winding = computeWindingNumber(position, triangleCount, triangles);

    return fabsf(winding) > 3.1415926f ? -best : best;
}

__global__ void sdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    const Hex32Field hexField,
    const CudaAABB domain
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = hexField.resolution.x * hexField.resolution.y * hexField.resolution.z;

    if (id >= cellCount)
        return;

    // Cell pos
    int cellX = id % hexField.resolution.x;
    int cellY = (id / hexField.resolution.x) % hexField.resolution.y;
    int cellZ = id / (hexField.resolution.x * hexField.resolution.y);

    // Cell size
    float3 cellSize = make_float3(
        (domain.max.x - domain.min.x) / hexField.resolution.x,
        (domain.max.y - domain.min.y) / hexField.resolution.y,
        (domain.max.z - domain.min.z) / hexField.resolution.z
    );

    // float3 cellThirds = make_float3(
    //     cellSize.x * (1.0 / 3.0f),
    //     cellSize.y * (1.0 / 3.0f),
    //     cellSize.z * (1.0 / 3.0f)
    // );

    float3 cellBasePos = make_float3(
        domain.min.x + cellX * cellSize.x,
        domain.min.y + cellY * cellSize.y,
        domain.min.z + cellZ * cellSize.z
    );

    // Calculate sdf values for each node of cell
    Hex32Cell* cell = &hexField.d_cells[id];

    for (int n = 0 ; n < 32 ; n++) {
        HexNode node = hex32Nodes[n];

        float3 localPos = make_float3(
            (node.xi   + 1.0f) * 0.5f,
            (node.eta  + 1.0f) * 0.5f,
            (node.zeta + 1.0f) * 0.5f
        );

        float3 worldPos = make_float3(
            cellBasePos.x + localPos.x * cellSize.x,
            cellBasePos.y + localPos.y * cellSize.y,
            cellBasePos.z + localPos.z * cellSize.z
        );

        cell->nodes[n] = calculateSDF(worldPos, triangleCount, triangles);
    }

    // int i = 0;
    // for (int z = 0 ; z < 4 ; z++) {
    //     for (int y = 0 ; y < 4 ; y++) {
    //         for (int x = 0 ; x < 4 ; x++) {
    //             int bx = (x == 0 || x == 3);
    //             int by = (y == 0 || y == 3);
    //             int bz = (z == 0 || z == 3);
    //
    //             // This node is a corner or edge node
    //             if (bx + by + bz >= 2) {
    //                 float3 nodePos = make_float3(
    //                     cellBasePos.x + x * cellThirds.x,
    //                     cellBasePos.y + y * cellThirds.y,
    //                     cellBasePos.z + z * cellThirds.z
    //                 );
    //
    //                 cell->nodes[i] = calculateSDF(nodePos, triangleCount, triangles);
    //                 cell->positions[i] = nodePos;
    //                 i++;
    //             }
    //         }
    //     }
    // }
}

void launchSdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    const Hex32Field& hexField,
    const CudaAABB& domain
) {
    int threadsPerBlock = 256;
    int cells = hexField.resolution.x * hexField.resolution.y * hexField.resolution.z;
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    sdfKernel<<<blocks, threadsPerBlock>>>(
        triangleCount,
        triangles,
        hexField,
        domain
    );
}

__global__ void initCuRand(curandState* states, unsigned long long seed, int n) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= n)
        return;

    curand_init(seed, id, 0, &states[id]);
}

__device__ float gamma(float restDensity, float h, float x) {
    if (x <= h)
        return restDensity * (1 - x / h);
    return 0;
}

__device__ float3 sampleSphere(curandState* state, float3 center, float radius) {
    float x = curand_normal(state);
    float y = curand_normal(state);
    float z = curand_normal(state);

    float3 dir = normalize3(make_float3(x, y, z));

    float r = cbrtf(curand_uniform(state)) * radius;

    return make_float3(
        center.x + dir.x * r,
        center.y + dir.y * r,
        center.z + dir.z * r
    );
}

__device__ float3 T(float3 min, float3 max, float3 position) {
    float3 s = max - min;
    float3 t = 2.0f * position - (max + min);

    return make_float3(
        t.x / s.x,
        t.y / s.y,
        t.z / s.z
    );
}

__device__ float shapeFunction(
    float xi,   float eta,   float zeta,        // Query positon
    float xi_i, float eta_i, float zeta_i,      // Node coords [-1, 1]
    bool isCorner
) {
    if (isCorner) {
        // Equation 9
        return (1.0f / 64.0f)
            * (1.0f + xi_i   * xi)
            * (1.0f + eta_i  * eta)
            * (1.0f + zeta_i * zeta)
            * (9.0f * (xi*xi + eta*eta + zeta*zeta) - 19.0f);
    } else {
        // Equation 10
        return (9.0f / 64.0f)
            * (1.0f - xi*xi)
            * (1.0f + 9.0f * xi_i * xi)
            * (1.0f + eta_i  * eta)
            * (1.0f + zeta_i * zeta);
    }
}

__device__ float interpolate(
    float xi, float eta, float zeta,
    float* nodeValues
) {
    float result = 0;

    for (int i = 0 ; i < 32 ; i++) {
        result += shapeFunction(
            xi, eta, zeta,
            hex32Nodes[i].xi, hex32Nodes[i].eta, hex32Nodes[i].zeta,
            hex32Nodes[i].isCorner
        ) * nodeValues[i];
    }

    return result;
}

__global__ void calcDensityMapKernel(
    curandState* states,
    const Hex32Field sdfField,
    const Hex32Field densityField,
    const CudaAABB domain,
    float restDensity,
    float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = sdfField.resolution.x * sdfField.resolution.y * sdfField.resolution.z;

    if (id >= cellCount)
        return;

    // Cell pos
    int cellX = id % sdfField.resolution.x;
    int cellY = (id / sdfField.resolution.x) % sdfField.resolution.y;
    int cellZ = id / (sdfField.resolution.x * sdfField.resolution.y);

    // Cell size
    float3 cellSize = make_float3(
        (domain.max.x - domain.min.x) / sdfField.resolution.x,
        (domain.max.y - domain.min.y) / sdfField.resolution.y,
        (domain.max.z - domain.min.z) / sdfField.resolution.z
    );

    // float3 cellThirds = make_float3(
    //     cellSize.x * (1.0 / 3.0f),
    //     cellSize.y * (1.0 / 3.0f),
    //     cellSize.z * (1.0 / 3.0f)
    // );

    float3 cellBasePos = make_float3(
        domain.min.x + cellX * cellSize.x,
        domain.min.y + cellY * cellSize.y,
        domain.min.z + cellZ * cellSize.z
    );

    Hex32Cell* cell = &sdfField.d_cells[id];

    for (int nodeIndex = 0 ; nodeIndex < 32 ; nodeIndex++) {
        HexNode node = hex32Nodes[nodeIndex];

        float3 localPos = make_float3(
            (node.xi   + 1.0f) * 0.5f,
            (node.eta  + 1.0f) * 0.5f,
            (node.zeta + 1.0f) * 0.5f
        );

        float3 nodePos = make_float3(
            cellBasePos.x + localPos.x * cellSize.x,
            cellBasePos.y + localPos.y * cellSize.y,
            cellBasePos.z + localPos.z * cellSize.z
        );

        float density = 0;

        for (int sample = 0 ; sample < 100 ; sample++) {
            float3 samplePosition = sampleSphere(&states[id], nodePos, h);

            // Clamp to domain
            samplePosition.x = fmaxf(domain.min.x, fminf(domain.max.x, samplePosition.x));
            samplePosition.y = fmaxf(domain.min.y, fminf(domain.max.y, samplePosition.y));
            samplePosition.z = fmaxf(domain.min.z, fminf(domain.max.z, samplePosition.z));

            // Find cell
            int sampleCellX = (int)((samplePosition.x - domain.min.x) / cellSize.x);
            int sampleCellY = (int)((samplePosition.y - domain.min.y) / cellSize.y);
            int sampleCellZ = (int)((samplePosition.z - domain.min.z) / cellSize.z);

            // Clamp cell indices to valid range
            sampleCellX = min(sampleCellX, sdfField.resolution.x - 1);
            sampleCellY = min(sampleCellY, sdfField.resolution.y - 1);
            sampleCellZ = min(sampleCellZ, sdfField.resolution.z - 1);

            // Get cell
            int cellId = sampleCellX
                     + sampleCellY * sdfField.resolution.x
                     + sampleCellZ * sdfField.resolution.x * sdfField.resolution.y;
            Hex32Cell* sampleCell = &sdfField.d_cells[cellId];

            // World pos of cell
            float3 sampleCellBase = make_float3(
                domain.min.x + sampleCellX * cellSize.x,
                domain.min.y + sampleCellY * cellSize.y,
                domain.min.z + sampleCellZ * cellSize.z
            );

            float3 sampleCellMax = make_float3(
                sampleCellBase.x + cellSize.x,
                sampleCellBase.y + cellSize.y,
                sampleCellBase.z + cellSize.z
            );

            float3 normalizedPos = T(sampleCellBase, sampleCellMax, samplePosition);
            float interpolateSampleSDF = interpolate(normalizedPos.x, normalizedPos.y, normalizedPos.z, sampleCell->nodes);

            if (interpolateSampleSDF < 0.0f) {
                // Gamma
                float g = gamma(restDensity, h, fabsf(interpolateSampleSDF));

                // Kernel
                float3 diff = nodePos - samplePosition;
                float w = poly6Kernel(make_float4(diff.x, diff.y, diff.z, 0.0), h);

                density += g * w;
            }
        }

        float V = (4.0 / 3.0f) * M_PI * h * h * h;
        density = density * (V / 100);

        densityField.d_cells[id].nodes[nodeIndex] = density;
    }
}

void launchCalcDensityMapKernel(
    const Hex32Field sdfField,
    const Hex32Field densityField,
    const CudaAABB domain,
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
    cudaDeviceSynchronize();

    calcDensityMapKernel<<<blocks, threadsPerBlock>>>(
        d_states,
        sdfField,
        densityField,
        domain,
        restDensity,
        h
    );
    cudaDeviceSynchronize();

    cudaFree(d_states);
}
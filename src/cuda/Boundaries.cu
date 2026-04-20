#include "Boundaries.h"
#include "Math.cuh"

#define IDX(x,y,z) ((x) + (y) * resolution + (z) * resolution * resolution)

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

__device__ bool rayIntersectsTriangle(
    float3 orig, float3 dir,
    float3 v0, float3 v1, float3 v2)
{
    const float EPS = 1e-6f;

    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;

    float3 p = cross(dir, e2);
    float det = dot(e1, p);

    if (fabs(det) < EPS)
        return false;

    float invDet = 1.0f / det;

    float3 t = orig - v0;

    float u = dot(t, p) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    float3 q = cross(t, e1);

    float v = dot(dir, q) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return false;

    float tHit = dot(e2, q) * invDet;

    return tHit > EPS;
}

__global__ void sdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    float* sdf,
    const int cellCount,
    const int resolution,
    const float3 gridMin,
    const float3 gridMax
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= cellCount)
        return;

    // Cell pos
    float cellX = id % resolution;
    float cellY = (id / resolution) % resolution;
    float cellZ = id / (resolution * resolution);

    // Global cell pos
    float3 cellSize = make_float3(
        (gridMax.x - gridMin.x) / (resolution - 1),
        (gridMax.y - gridMin.y) / (resolution - 1),
        (gridMax.z - gridMin.z) / (resolution - 1)
    );

    float3 pos = make_float3(
        gridMin.x + cellX * cellSize.x,
        gridMin.y + cellY * cellSize.y,
        gridMin.z + cellZ * cellSize.z
    );

    float best = 1e20;

    for (int i = 0 ; i < triangleCount ; i++) {
        float dist = distanceTriangle(pos, triangles[i].v0, triangles[i].v1, triangles[i].v2);

        if (dist < best)
            best = dist;
    }

    // Inside / Outside check
    int hitCount = 0;
    float3 rayDir = make_float3(1.0f, 0.0f, 0.0f);

    for (int i = 0 ; i < triangleCount ; i++) {
        if (rayIntersectsTriangle(pos, rayDir, triangles[i].v0, triangles[i].v1, triangles[i].v2))
            hitCount++;
    }

    bool inside = (hitCount % 2) == 1;

    sdf[id] = inside ? -best : best;
}

void launchSdfKernel(
    const int triangleCount,
    const Triangle* triangles,
    float* sdf,
    const int cellCount,
    const int resolution,
    const float3 gridMin,
    const float3 gridMax
) {
    int threadsPerBlock = 256;
    int blocks = (cellCount + threadsPerBlock - 1) / threadsPerBlock;

    sdfKernel<<<blocks, threadsPerBlock>>>(
        triangleCount,
        triangles,
        sdf,
        cellCount,
        resolution,
        gridMin,
        gridMax
    );
}
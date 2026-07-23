#pragma once

#include <cuda/cmath>
#include "Math.cuh"
#include "../../include/assets/Triangle.h"

__device__ inline float distanceTriangle(float3 p, float3 v1, float3 v2, float3 v3) {
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

__device__ inline float solidAngle(float3 position, float3 v0, float3 v1, float3 v2) {
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

__device__ inline float computeWindingNumber(float3 position, int triangleCount, const Triangle* triangles) {
    float winding = 0.0f;

    for (int i = 0; i < triangleCount; i++)
        winding += solidAngle(position, triangles[i].v0, triangles[i].v1, triangles[i].v2);

    return winding;
}

__device__ inline float calculateSDF(float3 position, int triangleCount, const Triangle* triangles) {
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
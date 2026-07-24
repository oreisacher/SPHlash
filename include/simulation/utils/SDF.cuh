#pragma once

#define GLM_FORCE_CUDA

#include "assets/Triangle.h"
#include <glm/geometric.hpp>
#include <glm/common.hpp>

__device__ inline float distanceTriangle(glm::vec3 p, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
    glm::vec3 v21 = v2 - v1;
    glm::vec3 v32 = v3 - v2;
    glm::vec3 v13 = v1 - v3;

    glm::vec3 p1 = p - v1;
    glm::vec3 p2 = p - v2;
    glm::vec3 p3 = p - v3;

    glm::vec3 normal = glm::cross(v21, v13);

    auto dot2 = [](const glm::vec3& v) {
        return glm::dot(v, v);
    };

    if (glm::sign(glm::dot(glm::cross(v21, normal), p1)) +
        glm::sign(glm::dot(glm::cross(v32, normal), p2)) +
        glm::sign(glm::dot(glm::cross(v13, normal), p3)) < 2.0f) {
        return sqrt(fminf(fminf(
            dot2(v21 * glm::clamp(glm::dot(v21, p1) / dot2(v21), 0.0f, 1.0f) - p1),
            dot2(v32 * glm::clamp(glm::dot(v32, p2) / dot2(v32), 0.0f, 1.0f) - p2)),
            dot2(v13 * glm::clamp(glm::dot(v13, p3) / dot2(v13), 0.0f, 1.0f) - p3)));
    }

    return sqrt(glm::dot(normal, p1) * glm::dot(normal, p1) / dot2(normal));
}

__device__ inline float solidAngle(glm::vec3 position, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
    const glm::vec3 a = glm::normalize(v0 - position);
    const glm::vec3 b = glm::normalize(v1 - position);
    const glm::vec3 c = glm::normalize(v2 - position);

    float numerator = glm::dot(a, glm::cross(b, c));

    float denom =
        1.0f +
        glm::dot(a, b) +
        glm::dot(b, c) +
        glm::dot(c, a);

    return 2.0f * atan2f(numerator, denom);
}

__device__ inline float computeWindingNumber(glm::vec3 position, int triangleCount, const Triangle* triangles) {
    float winding = 0.0f;

    for (int i = 0; i < triangleCount; i++)
        winding += solidAngle(position, triangles[i].v0, triangles[i].v1, triangles[i].v2);

    return winding;
}

__device__ inline float calculateSDF(glm::vec3 position, int triangleCount, const Triangle* triangles) {
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
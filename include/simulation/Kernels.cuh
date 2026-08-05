#pragma once

#include <math_constants.h>

#include "glm/ext/quaternion_geometric.hpp"

__device__ __host__ inline float cubicSplineKernel(float r, float h)
{
    float q = r / h;

    float sigma = 1.0f / (CUDART_PI_F * h * h * h);

    float value;

    if (q < 1.0f)
    {
        value = 1.0f
              - 1.5f * q * q
              + 0.75f * q * q * q;
    }
    else if (q < 2.0f)
    {
        float t = 2.0f - q;
        value = 0.25f * t * t * t;
    }
    else
    {
        value = 0.0f;
    }

    return sigma * value;
}

__device__ __host__ inline glm::vec3 cubicSplineKernelGradient(glm::vec3 r, float h)
{
    float dist = sqrtf(
        r.x * r.x +
        r.y * r.y +
        r.z * r.z
    );

    if (dist < 1e-6f)
        return glm::vec3(0.0f);

    float q = dist / h;

    float factor = 0.0f;

    if (q < 1.0f)
    {
        // d/dq (1 - 1.5q^2 + 0.75q^3)
        factor = -3.0f * q + 2.25f * q * q;
    }
    else if (q < 2.0f)
    {
        // d/dq (0.25(2-q)^3)
        float t = 2.0f - q;
        factor = -0.75f * t * t;
    }
    else
    {
        return glm::vec3(0.0f);
    }

    // 3D normalization
    float sigma = 1.0f / (CUDART_PI_F * h * h * h * h);

    return sigma * factor * (r / dist);
}

__device__ __host__ inline float poly6Kernel(glm::vec3 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    float h2 = h * h;

    if (r2 <= h2) {
        float x = h2 - r2;
        float h9 = h2 * h2 * h2 * h2 * h;
        return 315.0f / (64.0f * CUDART_PI_F * h9) * x * x * x;
    }

    return 0.0f;
}

__device__ inline float poly6Kernel(float r, float h) {
    float r2 = r * r;
    float h2 = h * h;

    if (r2 <= h2) {
        float x = h2 - r2;
        float h9 = h2 * h2 * h2 * h2 * h;
        return 315.0f / (64.0f * CUDART_PI_F * h9) * x * x * x;
    }

    return 0.0f;
}

__device__ inline glm::vec3 poly6KernelGradient(glm::vec3 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    float h2 = h * h;

    if (r2 < h2) {
        float x = h2 - r2;
        float h9 = h2 * h2 * h2 * h2 * h;
        float factor = (-945.0f / (32.0f * CUDART_PI_F * h9)) * x * x;
        return glm::vec3(
            factor * dist.x,
            factor * dist.y,
            factor * dist.z
        );
    }

    return glm::vec3(0.0, 0.0, 0.0);
}

__device__ inline float poly6KernelLaplacian(glm::vec3 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    float h2 = h * h;

    if (r2 < h2) {
        float h9 = h2 * h2 * h2 * h2 * h;
        float term1 = 945.0f / (32.0f * CUDART_PI_F * h9);
        float term2 = h2 - r2;
        float term3 = 7.0f * r2 - 3.0f * h2;

        return term1 * term2 * term3;
    }

    return 0.0f;
}

__device__ inline glm::vec3 spikyKernelGradient(glm::vec3 dist, float h) {
    float r = sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

    if (r > 0.0f && r < h) {
        float x = h - r;
        float h6 = h * h * h * h * h * h;
        float factor = (-45.0f / (CUDART_PI_F * h6)) * x * x;
        return factor * (dist / r);
    }

    return glm::vec3(0.0f, 0.0f, 0.0f);
}

__device__ inline float viscosityLaplacianKernel(glm::vec3 dist, float h) {
    float r = sqrt(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

    if (r <= h) {
        float h6 = h * h * h * h * h * h;
        return (45.0f / (CUDART_PI_F * h6)) * (h - r);
    }

    return 0.0f;
}
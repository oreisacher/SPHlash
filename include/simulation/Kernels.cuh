#pragma once

#include <math_constants.h>

__device__ inline float poly6Kernel(float4 dist, float h) {
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

__device__ inline float4 poly6KernelGradient(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    float h2 = h * h;

    if (r2 < h2) {
        float x = h2 - r2;
        float h9 = h2 * h2 * h2 * h2 * h;
        float factor = (-945.0f / (32.0f * CUDART_PI_F * h9)) * x * x;
        return make_float4(
            factor * dist.x,
            factor * dist.y,
            factor * dist.z,
            0.0
        );
    }

    return make_float4(0.0, 0.0, 0.0, 0.0);
}

__device__ inline float poly6KernelLaplacian(float4 dist, float h) {
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

__device__ inline float4 spikyKernelGradient(float4 dist, float h) {
    float r = sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

    if (r > 0.0f && r < h) {
        float x = h - r;
        float h6 = h * h * h * h * h * h;
        float factor = (-45.0f / (CUDART_PI_F * h6)) * x * x;
        return make_float4(
            factor * (dist.x / r),
            factor * (dist.y / r),
            factor * (dist.z / r),
            0.0f
        );
    }

    return make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

__device__ inline float viscosityLaplacianKernel(float4 dist, float h) {
    float r = sqrt(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

    if (r <= h) {
        float h6 = h * h * h * h * h * h;
        return (45.0f / (CUDART_PI_F * h6)) * (h - r);
    }

    return 0.0f;
}
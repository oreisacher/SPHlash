#ifndef SPHLASH_KERNELS
#define SPHLASH_KERNELS

#include <cuda_runtime.h>
#include <math_constants.h>
#include <math.h>
#include "glm/vec3.hpp"

inline const char* kernelNames[] = {
    "Cubic Spline",
    "Poly6",
    "Spiky",
    "WendlandC2"
};

enum class KernelType {
    CubicSpline,
    Poly6,
    Spiky,
    WendlandC2
};

struct KernelSettings {
    KernelType density          = KernelType::CubicSpline;
    KernelType pressure         = KernelType::CubicSpline;
    KernelType viscosity        = KernelType::CubicSpline;
    KernelType surfaceTension   = KernelType::CubicSpline;
};

struct CubicSpline {
    static constexpr float SupportRadius = 2.0f;

    __device__ __host__ static float W(float r, float h) {
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

    __device__ __host__ static glm::vec3 gradW(glm::vec3 r, float h) {
        float dist = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);

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
};

struct Poly6 {
    static constexpr float SupportRadius = 1.0f;

    __device__ __host__ static float W(float r2, float h) {
        float h2 = h * h;

        if (r2 <= h2) {
            float x = h2 - r2;
            float h9 = h2 * h2 * h2 * h2 * h;
            return 315.0f / (64.0f * CUDART_PI_F * h9) * x * x * x;
        }

        return 0.0f;
    }

    __device__ __host__ static glm::vec3 gradW(glm::vec3 r, float h) {
        float r2 = r.x * r.x + r.y * r.y + r.z * r.z;
        float h2 = h * h;

        if (r2 < h2) {
            float x = h2 - r2;
            float h9 = h2 * h2 * h2 * h2 * h;
            float factor = (-945.0f / (32.0f * CUDART_PI_F * h9)) * x * x;
            return glm::vec3(
                factor * r.x,
                factor * r.y,
                factor * r.z
            );
        }

        return glm::vec3(0.0, 0.0, 0.0);
    }
};

struct Spiky {
    static constexpr float SupportRadius = 1.0f;

    __device__ __host__ static float W(float r, float h) {
        if (r >= 0.0f && r <= h) {
            float x = h - r;
            float h6 = h * h * h * h * h * h;

            return (15.0f / (CUDART_PI_F * h6)) * x * x * x;
        }

        return 0.0f;
    }

    __device__ __host__ static glm::vec3 gradW(glm::vec3 dist, float h) {
        float r = sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

        if (r > 0.0f && r < h) {
            float x = h - r;
            float h6 = h * h * h * h * h * h;
            float factor = (-45.0f / (CUDART_PI_F * h6)) * x * x;
            return factor * (dist / r);
        }

        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
};

struct WendlandC2 {
    static constexpr float SupportRadius = 2.0f;

    __device__ __host__ static float W(float r, float h) {
        float q = r / h;

        if (q <= 2.0f)
        {
            float term = 1.0f - 0.5f * q;

            return (21.0f / (16.0f * CUDART_PI_F * h*h*h)) *
                   term*term*term*term *
                   (2.0f*q + 1.0f);
        }

        return 0.0f;
    }


    __device__ __host__  static glm::vec3 gradW(glm::vec3 dist, float h) {
        float r = sqrtf(
            dist.x*dist.x +
            dist.y*dist.y +
            dist.z*dist.z
        );

        if (r < 1e-6f || r > 2.0f*h)
            return glm::vec3(0.0f);


        float q = r / h;

        float term = 1.0f - 0.5f*q;

        float factor =
            -(105.0f / (16.0f * CUDART_PI_F * h*h*h*h)) *
            q *
            term*term*term;


        return factor * (dist / r);
    }
};

__host__ __device__ inline float getKernelSupportRadius(KernelType type) {
    switch (type) {
        case KernelType::CubicSpline:
            return CubicSpline::SupportRadius;

        case KernelType::Poly6:
            return Poly6::SupportRadius;

        case KernelType::Spiky:
            return Spiky::SupportRadius;

    case KernelType::WendlandC2:
        return WendlandC2::SupportRadius;
    }

    return 0.0f;
}

__host__ __device__ inline float getMaxSupportRadius(const KernelSettings& settings) {
    float maxRadius = 0.0f;

    maxRadius = fmaxf(maxRadius, getKernelSupportRadius(settings.density));
    maxRadius = fmaxf(maxRadius, getKernelSupportRadius(settings.pressure));
    maxRadius = fmaxf(maxRadius, getKernelSupportRadius(settings.viscosity));
    maxRadius = fmaxf(maxRadius, getKernelSupportRadius(settings.surfaceTension));

    return maxRadius;
}

__host__ __device__ inline float evaluateKernel(KernelType type, float r, float h) {
    switch (type) {
        case KernelType::CubicSpline:
            return CubicSpline::W(r, h);

        case KernelType::Poly6:
            return Poly6::W(r * r, h);

        case KernelType::Spiky:
            return Spiky::W(r, h);

        case KernelType::WendlandC2:
            return WendlandC2::W(r, h);
    }

    return 0.0f;
}

__host__ __device__ inline glm::vec3 evaluateKernelGradient(KernelType type, glm::vec3 r, float h) {
    switch (type) {
        case KernelType::CubicSpline:
            return CubicSpline::gradW(r, h);

        case KernelType::Poly6:
            return Poly6::gradW(r, h);

        case KernelType::Spiky:
            return Spiky::gradW(r, h);

        case KernelType::WendlandC2:
            return WendlandC2::gradW(r, h);
    }

    return glm::vec3(0.0);
}

#endif
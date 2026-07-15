#include "SimulationMetrics.h"

#include <thrust/execution_policy.h>
#include <thrust/transform_reduce.h>
#include "Math.cuh"

struct DensityError {
    const float4* properties;
    float restDensity;

    __device__ float operator()(const int i) const {
        return fabsf(properties[i].y - restDensity) / restDensity;
    }
};

float calculateAverageDensityError(
    const int numParticles,
    const float4* properties,
    const float p_0
) {
    const thrust::counting_iterator<int> begin(0);
    const thrust::counting_iterator<int> end(numParticles);

    float averageDensityError = thrust::transform_reduce(
        begin,
        end,
        DensityError { properties, p_0 },
        0.0f,
        thrust::plus<float>()
    );

    return averageDensityError / numParticles;
}

struct DensitySquaredDeviation {
    const float4* properties;
    const float avgDensity;

    __device__ float operator()(const int i) const {
        float diff = properties[i].y - avgDensity;
        return diff * diff;
    }
};

float calculateDensityStandardDeviation(
    const int numParticles,
    const float4* properties,
    const float avgDensity
) {
    const thrust::counting_iterator<int> begin(0);
    const thrust::counting_iterator<int> end(numParticles);

    float densityStdDeviation = thrust::transform_reduce(
        begin,
        end,
        DensitySquaredDeviation { properties, avgDensity },
        0.0f,
        thrust::plus<float>()
    );

    return sqrt(densityStdDeviation / numParticles);
}

struct AccelerationSq {
    const float4* pf;
    const float4* vf;
    const float4* stf;

    __device__ float operator()(int i) const {
        float4 f = pf[i] + vf[i] + stf[i];
        return f.x * f.x + f.y * f.y + f.z * f.z;
    }
};

float calculateMaxAcceleration(
    int numParticles,
    const float4* pressureForce,
    const float4* viscosityForce,
    const float4* surfaceTensionForce
) {
    const thrust::counting_iterator<int> begin(0);
    const thrust::counting_iterator<int> end(numParticles);

    const float maxAcc = thrust::transform_reduce(
        begin,
        end,
        AccelerationSq{pressureForce, viscosityForce, surfaceTensionForce},
        0.0f,
        thrust::maximum<float>()
    );

    return sqrtf(maxAcc);
}

struct DensitySum {
    const float4* properties;

    __device__ float operator()(const int i) const {
        return properties[i].y;
    }
};

float calculateAverageDensity(
    int numParticles,
    const float4* properties
) {
    const thrust::counting_iterator<int> begin(0);
    const thrust::counting_iterator<int> end(numParticles);

    float totalDensity = thrust::transform_reduce(
        begin,
        end,
        DensitySum { properties },
        0.0f,
        thrust::plus<float>()
    );

    return totalDensity / static_cast<float>(numParticles);
}

struct HeightCount {
    float height;
    int count;

    __host__ __device__
    HeightCount(float h = 0.0f, int c = 0) : height(h), count(c) {}
};

struct TopSurfaceHeight {
    const float4* position;
    const float4* flags;

    __device__ HeightCount operator()(const int i) const {
        if (flags[i].y >= 0.5f) {
            return HeightCount(position[i].y, 1);
        }

        return HeightCount(0.0f, 0);
    }
};

struct HeightCountAdd {
    __host__ __device__
    HeightCount operator()(const HeightCount& a, const HeightCount& b) const {
        return HeightCount(
            a.height + b.height,
            a.count + b.count
        );
    }
};

float calculateAverageTopSurfaceHeight(
    int numParticles,
    const float4* position,
    const float4* flags
) {
    auto begin = thrust::counting_iterator<int>(0);
    auto end = begin + numParticles;

    HeightCount result = thrust::transform_reduce(
        begin,
        end,
        TopSurfaceHeight{position, flags},
        HeightCount(),
        HeightCountAdd()
    );

    if (result.count == 0)
        return 0.0f;

    return result.height / static_cast<float>(result.count);
}
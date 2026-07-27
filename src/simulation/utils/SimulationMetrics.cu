#include "simulation/utils/SimulationMetrics.cuh"
#include "glm/vec3.hpp"
#include "thrust/transform_reduce.h"

__device__
Metrics operator+(const Metrics& lhs, const Metrics& rhs) {
    Metrics result;

    result.count = lhs.count + rhs.count;

    result.position = lhs.position + rhs.position;
    result.velocity = lhs.velocity + rhs.velocity;
    result.pressureForce = lhs.pressureForce + rhs.pressureForce;
    result.viscosityForce = lhs.viscosityForce + rhs.viscosityForce;
    result.surfaceTensionForce = lhs.surfaceTensionForce + rhs.surfaceTensionForce;

    result.density = lhs.density + rhs.density;
    result.densityError = lhs.densityError + rhs.densityError;
    result.pressure = lhs.pressure + rhs.pressure;

    return result;
}

__global__ void metricsKernel (
    int numParticles,
    ParticlesGPU particles,
    int filter,
    float restDensity,
    AABBView area,
    Metrics* blockResults
) {
    __shared__ Metrics shared[256];

    int id = blockIdx.x * blockDim.x + threadIdx.x;

    Metrics local{};

    if (id < numParticles) {
        bool include = true;

        glm::vec3 pos = particles.d_position[id];

        // Only consider particles inside query bb
        if (pos.x < area.min.x || pos.x > area.max.x ||
            pos.y < area.min.y || pos.y > area.max.y ||
            pos.z < area.min.z || pos.z > area.max.z )
            include = false;

        // Top Surface Filter
        if (filter == 1 && particles.d_flags[id].y != 1.0)
            include = false;

        if (include) {
            local.count = 1;

            local.position = pos;
            local.velocity = particles.d_velocity[id];
            local.pressureForce = particles.d_pressureForce[id];
            local.viscosityForce = particles.d_viscosityForce[id];
            local.surfaceTensionForce = particles.d_surfaceTensionForce[id];
            local.density = particles.d_properties[id].y;
            local.pressure = particles.d_properties[id].z;

            local.densityError = fabsf(particles.d_properties[id].y - restDensity) / restDensity;
        }
    }

    shared[threadIdx.x] = local;
    __syncthreads();

    // Parallel reduction
    for (int stride = 256 / 2; stride > 0; stride >>= 1) {
        if (threadIdx.x < stride) {
            shared[threadIdx.x] = shared[threadIdx.x] + shared[threadIdx.x + stride];
        }

        __syncthreads();
    }

    if (threadIdx.x == 0)
        blockResults[blockIdx.x] = shared[0];
}
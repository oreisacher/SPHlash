//
// Created by ruut on 7/15/26.
//

#ifndef SPHLASH_SIMULATIONMETRICS_H
#define SPHLASH_SIMULATIONMETRICS_H

#include "glm/vec3.hpp"
#include "simulation/scene/nodes/QueryNode.h"

struct MetricsAdd {
    __host__ __device__ Metrics operator()(const Metrics& a, const Metrics& b) const {
        Metrics r;

        r.count = a.count + b.count;
        r.position = a.position + b.position;
        r.velocity = a.velocity + b.velocity;
        r.pressureForce = a.pressureForce + b.pressureForce;
        r.viscosityForce = a.viscosityForce + b.viscosityForce;
        r.surfaceTensionForce = a.surfaceTensionForce + b.surfaceTensionForce;
        r.density = a.density + b.density;
        r.pressure = a.pressure + b.pressure;
        r.densityError = a.densityError + b.densityError;

        return r;
    }
};

__global__ void metricsKernel (
    int numParticles,
    ParticlesGPU particles,
    int filter,
    float restDensity,
    AABBView area,
    Metrics* blockResults
);

#endif //SPHLASH_SIMULATIONMETRICS_H

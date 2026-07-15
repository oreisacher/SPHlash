//
// Created by ruut on 7/15/26.
//

#ifndef SPHLASH_SIMULATIONMETRICS_H
#define SPHLASH_SIMULATIONMETRICS_H

#include <vector_types.h>

float calculateAverageDensityError(
    const int numParticles,
    const float4* properties,
    const float p_0
);

float calculateDensityStandardDeviation(
    const int numParticles,
    const float4* properties,
    const float avgDensity
);

float calculateMaxAcceleration(
    int numParticles,
    const float4* pressureForce,
    const float4* viscosityForce,
    const float4* surfaceTensionForce
);

float calculateAverageDensity(
    int numParticles,
    const float4* properties
);

float calculateAverageTopSurfaceHeight(
    int numParticles,
    const float4* position,
    const float4* flags
);

#endif //SPHLASH_SIMULATIONMETRICS_H

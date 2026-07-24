//
// Created by ruut on 7/15/26.
//

#ifndef SPHLASH_SIMULATIONMETRICS_H
#define SPHLASH_SIMULATIONMETRICS_H

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

float calculateAverageDensityError(
    int numParticles,
    const glm::vec4* properties,
    float p_0
);

float calculateDensityStandardDeviation(
    int numParticles,
    const glm::vec4* properties,
    float avgDensity
);

float calculateMaxAcceleration(
    int numParticles,
    const glm::vec3* pressureForce,
    const glm::vec3* viscosityForce,
    const glm::vec3* surfaceTensionForce
);

float calculateAverageDensity(
    int numParticles,
    const glm::vec4* properties
);

float calculateAverageTopSurfaceHeight(
    int numParticles,
    const glm::vec3* position,
    const glm::vec4* flags
);

#endif //SPHLASH_SIMULATIONMETRICS_H

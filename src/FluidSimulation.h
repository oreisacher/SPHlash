//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATION_H
#define FLUIDSIMSPH_FLUIDSIMULATION_H

#include <vector>
#include "glm/vec2.hpp"

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    float mass;

    // Per-Frame data
    float density;
    float pressure;

    glm::vec2 pressureForce;
    glm::vec2 viscosityForce;
};

class FluidSimulation {
public:
    FluidSimulation(int particleCount, int xMin, int yMin, int xMax, int yMax);

    void update(float deltaTime);

    int getParticleCount() const;
    const Particle* getData() const;
private:
    // Calculations
    void calculateDensity();
    void calculatePressure();
    void calculatePressureForce();
    void calculateViscosityForce();

    void updateParticles(float deltaTime);

    // Kernels
    float poly6Kernel3D(float r) const;
    float poly6Kernel2D(glm::vec2 dist) const;
    float poly6LaplacianKernel2D(glm::vec2 dist) const;

    float spikyKernel3D(float r) const;
    float spikyKernel2D(glm::vec2 dist) const;
    glm::vec2 spikyKernelGradient2D(glm::vec2 dist) const;

    float viscosityKernel2D(float r) const;
    float viscosityLaplacianKernel2D(glm::vec2 dist) const;

    std::vector<Particle> particles;

    // Constants
    float h = 0.13;
    float p_0 = 1000;
    float k = 10000;
    float mu = 0.01;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATION_H
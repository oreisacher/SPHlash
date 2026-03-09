//
// Created by ruut on 3/9/26.
//

#define GLM_ENABLE_EXPERIMENTAL

#include "FluidSimulation.h"
#include "glm/glm.hpp"
#include <random>
#include <iostream>
#include "glm/gtx/norm.hpp"

FluidSimulation::FluidSimulation(int particleCount, int xMin, int yMin, int xMax, int yMax) {
    // Setup particles
    particles.reserve(particleCount);

    const int cols = std::ceil(std::sqrt(particleCount));
    const int rows = std::ceil(static_cast<float>(particleCount) / cols);

    float stepSizeX = static_cast<float>(xMax - xMin) / (cols - 1);
    float stepSizeY = rows > 1 ? static_cast<float>(yMax - yMin) / (rows - 1) : 0;

    int count = 0;

    for (int r = 0; r < rows && count < particleCount; r++) {
        for (int c = 0; c < cols && count < particleCount; c++) {
            float x = xMin + c * stepSizeX;
            float y = yMin + r * stepSizeY;

            particles.push_back({
                { x, y },
                glm::vec2(0.0),
                9
            });

            count++;
        }
    }

    // Update h
    h = 1.6f * std::min(stepSizeX, stepSizeY);
    std::cout << "H: " << h << std::endl;

    // Reset density
    calculateDensity();
    p_0 = particles[particleCount / 2].density;
    std::cout << "P_0: " << p_0 << std::endl;

    std::cout << p_0 * std::min(stepSizeX, stepSizeY) * std::min(stepSizeX, stepSizeY);
}

void FluidSimulation::update(float deltaTime) {
    // Determine density for each particle
    calculateDensity();

    // Determine pressure for each particle
    //  Eq. 12
    calculatePressure();

    // Determine pressure force for each particle
    //  Eq. 10
    //  W_Spiky
    calculatePressureForce();

    // Determine viscosity force for each particle
    //  Eq. 14
    //  W_Viscosity
    calculateViscosityForce();

    // Determine surface tension force for each particle
    //  Eq. 19

    // Determine external forces

    // Determine acceleration
    //  Eq. 8

    // Update velocity
    updateParticles(deltaTime);
}

int FluidSimulation::getParticleCount() const {
    return particles.size();
}

const Particle* FluidSimulation::getData() const {
    return particles.data();
}

void FluidSimulation::calculateDensity() {
    for (auto& p : particles) {
        p.density = 0.0f;

        for (auto& other : particles) {
            p.density += other.mass * poly6Kernel2D(p.position - other.position);
        }
    }
}

void FluidSimulation::calculatePressure() {
    for (auto& p : particles) {
        p.pressure = k * (p.density - p_0);
        // p.pressure = std::max(0.0f, k * (p.density - p_0));
    }
}

void FluidSimulation::calculatePressureForce() {
    for (auto& p : particles) {
        p.pressureForce = glm::vec2(0.0f);

        for (auto& other : particles) {
            if (&other == &p) continue;

            p.pressureForce += other.mass * ((p.pressure + other.pressure) / (2.0f * other.density)) * spikyKernelGradient2D(p.position - other.position);
        }

        p.pressureForce *= -1;
    }
}

void FluidSimulation::calculateViscosityForce() {
    for (auto& p : particles) {
        p.viscosityForce = glm::vec2(0.0f);

        for (auto& other : particles) {
            if (&other == &p) continue;

            p.viscosityForce += other.mass * ((other.velocity - p.velocity) / other.density) * viscosityLaplacianKernel2D(p.position - other.position);
        }

        p.viscosityForce *= mu;
    }
}

void FluidSimulation::updateParticles(float deltaTime) {
    for (auto& p : particles) {
        glm::vec2 force = p.pressureForce + p.viscosityForce + glm::vec2(0.0f, -9.81f);
        glm::vec2 acceleration = force / p.density;

        p.velocity += acceleration * deltaTime;
        p.position += p.velocity * deltaTime;

        // Simple boundaries
        if (p.position.x < 0 ) {
            p.position.x = 0;
            p.velocity.x *= -1;
        }

        if (p.position.x > 100 ) {
            p.position.x = 100;
            p.velocity.x *= -1;
        }

        if (p.position.y < 0 ) {
            p.position.y = 0;
            p.velocity.y *= -1;
        }

        if (p.position.y > 100 ) {
            p.position.y = 100;
            p.velocity.y *= -1;
        }
    }
}

float FluidSimulation::poly6Kernel3D(float r) const {
    float h2 = h * h;
    float r2 = r * r;

    if (r2 >= 0.0f && r2 <= h2) {
        return (315.0f / (64.0f * std::numbers::pi * pow(h, 9))) * pow(h2 - r2, 3);
    }

    return 0.0f;
}

float FluidSimulation::poly6Kernel2D(glm::vec2 dist) const {
    float h2 = h * h;
    float r2 = glm::length2(dist);

    if (r2 <= h2) {
        return 4.0f / (std::numbers::pi * pow(h, 8)) * pow(h2 - r2, 3);
    }

    return 0.0f;
}

float FluidSimulation::poly6LaplacianKernel2D(glm::vec2 dist) const {
    float h2 = h * h;
    float r2 = glm::length2(dist);

    if (r2 <= h2) {
        return -48.0f / (std::numbers::pi * pow(h, 8)) * (h2 - r2) * (h2 - 3.0 * r2);
    }

    return 0.0f;
}

float FluidSimulation::spikyKernel3D(float r) const {
    if (r >= 0.0f && r <= h) {
        return (15.0f / (std::numbers::pi * pow(h, 6))) * pow(h - r, 3);
    }

    return 0.0f;
}

float FluidSimulation::spikyKernel2D(glm::vec2 dist) const {
    float r = std::abs(glm::length(dist));

    if (r <= h) {
        return 10.0f / (std::numbers::pi * pow(h, 5)) * pow(h - r, 3);
    }

    return 0.0f;
}

glm::vec2 FluidSimulation::spikyKernelGradient2D(glm::vec2 dist) const {
    float r = std::abs(glm::length(dist));

    if (r > 0 && r <= h) {
        float x = h - r;
        float factor = -30.0f / (std::numbers::pi * pow(h, 5)) * x * x;
        return factor * (dist / r);
    }

    return glm::vec2(0.0f);
}

float FluidSimulation::viscosityKernel2D(float r) const {
    if (r >= 0.0f && r <= h) {
        return
            (15.0f / (2.0f * std::numbers::pi * pow(h, 3)))
        *   (   -pow(r, 3) / (2.0f * pow(h, 3))
                + pow(r, 2) / pow(h, 2)
                + h / (2.0f * r)
                - 1
            );
    }

    return 0.0f;
}

float FluidSimulation::viscosityLaplacianKernel2D(glm::vec2 dist) const {
    float r = std::abs(glm::length(dist));

    if (r <= h) {
        return 45.0f / (std::numbers::pi * pow(h, 6)) * (h - r);
    }

    return 0.0f;
}

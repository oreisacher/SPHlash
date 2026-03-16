//
// Created by ruut on 3/9/26.
//

#define GLM_ENABLE_EXPERIMENTAL

#include "FluidSimulation.h"
#include "glm/glm.hpp"
#include <random>
#include <iostream>
#include "glm/gtx/norm.hpp"
#include <chrono>
#include <omp.h>

FluidSimulation::FluidSimulation(int particleCount, float xMin, float yMin, float xMax, float yMax) {
    // Setup particles
    particles.reserve(particleCount);

    const int cols = std::ceil(std::sqrt(particleCount));
    const int rows = std::ceil(static_cast<float>(particleCount) / cols);

    float stepSizeX = static_cast<float>(xMax - xMin) / (cols - 1);
    float stepSizeY = rows > 1 ? static_cast<float>(yMax - yMin) / (rows - 1) : 0;

    int count = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.01f, 0.01f);

    for (int r = 0; r < rows && count < particleCount; r++) {
        for (int c = 0; c < cols && count < particleCount; c++) {
            float x = xMin + c * stepSizeX + dist(gen);
            float y = yMin + r * stepSizeY + dist(gen);

            particles.push_back({
                { x, y },
                glm::vec2(0.0),
                1,
            });

            count++;
        }
    }

    // Update h
    // h = 25.0f;
    h = 2.0f * std::min(stepSizeX, stepSizeY);
    h2 = h * h;
    h3 = pow(h, 3);
    h5 = pow(h, 5);
    h6 = pow(h, 6);
    h8 = pow(h, 8);

    std::cout << "H: " << h << std::endl;
    std::cout << stepSizeX << " " << stepSizeY << std::endl;

    // Setup grid
    grid.cellSize = h;

    for (int i = 0 ; i < particles.size(); i++) {
        grid.insert(i, particles[i].position);
    }

    // Reset density
    calculateDensityPressure();
    // p_0 = particles[particleCount / 2].density;
    p_0 = 1.0f / (h * h);
    std::cout << "P_0: " << p_0 << std::endl;
    // p_0 = 0.0004f;
}

void FluidSimulation::update(float deltaTime) {
    // Rebuild grid data structure
    grid.clear();

    for (int i = 0 ; i < particles.size(); i++) {
        grid.insert(i, particles[i].position);
    }

    // Determine density for each particle
    // Determine pressure for each particle
    //  Eq. 12

    calculateDensityPressure();

    calculateSPHForces();

    // Determine pressure force for each particle
    //  Eq. 10
    //  W_Spiky
    // calculatePressureForce();

    // Determine viscosity force for each particle
    //  Eq. 14
    //  W_Viscosity
    // calculateViscosityForce();

    // Determine surface tension force for each particle
    //  Eq. 19
    // calculateSurfaceTensionForce();

    // Determine external forces
    applyMouseForce();

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

void FluidSimulation::updateBoundaries(float width, float height) {
    this->width = width;
    this->height = height;
}

void FluidSimulation::updateMousePos(float x, float y) {
    this->mouseX = x;

    // Flip y position as simulation as y0 at the bottom and not the top
    this->mouseY = height - y;
}

void FluidSimulation::updateMouseForceDirection(float direction) {
    this->mouseForceDirection = direction;
}

void FluidSimulation::updateMouseRadius(float direction) {
    this->mouseRadius += 0.01f * direction;
}

float FluidSimulation::getMouseRadius() const { return mouseRadius; }

void FluidSimulation::calculateDensityPressure() {
    #pragma omp parallel for
    for (int i = 0 ; i < particles.size() ; i++) {
        Particle& p = particles[i];

        p.density = 0.0f;

        for (auto o : grid.getNeighbors(p.position)) {
            Particle& other = particles[o];
            p.density += other.mass * poly6Kernel2D(p.position - other.position);
        }

        p.pressure = k * (p.density - p_0);
        p.pressure = std::max(0.0f, p.pressure);
    }
}

void FluidSimulation::calculateSPHForces() {
    #pragma omp parallel for
    for (int i = 0 ; i < particles.size() ; i++) {
        Particle& p = particles[i];

        p.pressureForce = glm::vec2(0.0f);
        p.viscosityForce = glm::vec2(0.0f);
        p.surfaceTensionForce = glm::vec2(0.0f);

        glm::vec2 n = glm::vec2(0.0f);
        float c = 0.0f;

        for (auto o : grid.getNeighbors(p.position)) {
            if (i == o) continue;

            Particle& other = particles[o];

            p.pressureForce += other.mass * ((p.pressure + other.pressure) / (2.0f * other.density)) * spikyKernelGradient2D(p.position - other.position);
            p.viscosityForce += other.mass * ((other.velocity - p.velocity) / other.density) * viscosityLaplacianKernel2D(p.position - other.position);
            n += other.mass * (1.0f / other.density) * poly6KernelGradient2D(p.position - other.position);
            c += other.mass * (1.0f / other.density) * poly6LaplacianKernel2D(p.position - other.position);
        }

        p.pressureForce *= -1.0f;
        p.viscosityForce *= mu;

        if (glm::length(n) > 10.0f)
            p.surfaceTensionForce = -1.0f * tension * c * (n / glm::length(n));
    }
}

void FluidSimulation::calculatePressureForce() {
    for (int i = 0 ; i < particles.size() ; i++) {
        Particle& p = particles[i];

        p.pressureForce = glm::vec2(0.0f);

        for (auto o : grid.getNeighbors(p.position)) {
            if (i == o) continue;

            Particle& other = particles[o];

            p.pressureForce += other.mass * ((p.pressure + other.pressure) / (2.0f * other.density)) * spikyKernelGradient2D(p.position - other.position);
        }

        p.pressureForce *= -1.0f;
    }
}

void FluidSimulation::calculateViscosityForce() {
    for (int i = 0 ; i < particles.size() ; i++) {
        Particle& p = particles[i];

        p.viscosityForce = glm::vec2(0.0f);

        for (auto o : grid.getNeighbors(p.position)) {
            if (i == o) continue;

            Particle& other = particles[o];

            p.viscosityForce += other.mass * ((other.velocity - p.velocity) / other.density) * viscosityLaplacianKernel2D(p.position - other.position);
        }

        p.viscosityForce *= mu;
    }
}

void FluidSimulation::calculateSurfaceTensionForce() {
    for (int i = 0 ; i < particles.size() ; i++) {
        Particle& p = particles[i];

        p.surfaceTensionForce = glm::vec2(0.0f);

        glm::vec2 n = glm::vec2(0.0f);
        float c = 0.0f;

        for (auto o : grid.getNeighbors(p.position)) {
            if (i == o) continue;

            Particle& other = particles[o];

            n += other.mass * (1.0f / other.density) * poly6KernelGradient2D(p.position - other.position);
            c += other.mass * (1.0f / other.density) * poly6LaplacianKernel2D(p.position - other.position);
        }

        std::cout << glm::length(n) << std::endl;
        if (glm::length(n) > 0.1f)
            p.surfaceTensionForce = -1.0f * tension * c * (n / glm::length(n));
    }
}

void FluidSimulation::applyMouseForce() {
    #pragma omp parallel for
    for (int i = 0; i < particles.size(); i++) {
        Particle& p = particles[i];

        glm::vec2 dir = p.position - glm::vec2(mouseX, mouseY);
        float dist2 = glm::length2(dir);

        if (dist2 < mouseRadius * mouseRadius) {
            float dist = std::sqrt(dist2);

            glm::vec2 forceDir = (dist > 0.0001f) ? dir / dist : glm::vec2(0.0f);

            float falloff = 1.0f - (dist / mouseRadius);
            p.velocity += mouseForceDirection * forceDir * mouseRadius * falloff;
        }
    }
}

void FluidSimulation::updateParticles(float deltaTime) {
    #pragma omp parallel for
    for (int i = 0; i < particles.size(); i++) {
        Particle& p = particles[i];

        glm::vec2 force = p.pressureForce + p.viscosityForce + p.surfaceTensionForce;
        glm::vec2 acceleration = force / p.density;// + glm::vec2(0.0f, -9.81f);

        p.velocity += acceleration * (0.5f * deltaTime);
        p.position += p.velocity * deltaTime;
        p.velocity += acceleration * (0.5f * deltaTime);

        // Simple boundaries
        if (p.position.x < 0) {
            p.position.x = 0;
            p.velocity.x *= -1.0f;
        }

        if (p.position.x > width) {
            p.position.x = width;
            p.velocity.x *= -1.0f;
        }

        if (p.position.y < 0) {
            p.position.y = 0;
            p.velocity.y *= -1.0f;
        }

        if (p.position.y > height) {
            p.position.y = height;
            p.velocity.y *= -1.0f;
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
    float r2 = glm::length2(dist);

    if (r2 <= h2) {
        float x = h2 - r2;
        return 4.0f / (std::numbers::pi * h8) * x * x * x;
    }

    return 0.0f;
}

glm::vec2 FluidSimulation::poly6KernelGradient2D(glm::vec2 dist) const {
    float r2 = glm::length2(dist);

    if (r2 > 0 && r2 < h * h) {
        float x = h2- r2;
        float factor = -24.0f / (std::numbers::pi * h8) * x * x;
        return factor * dist;
    }

    return glm::vec2(0.0f);
}

// float FluidSimulation::poly6LaplacianKernel2D(glm::vec2 dist) const {
//     float r2 = glm::length2(dist);
//
//     if (r2 <= h2) {
//         return -48.0f / (std::numbers::pi * h8) * (h2 - r2) * (h2 - 3.0 * r2);
//     }
//
//     return 0.0f;
// }

float FluidSimulation::poly6LaplacianKernel2D(glm::vec2 dist) const {
    float r2 = glm::length2(dist);

    if (r2 < h2 && r2 > 0.0f) {
        float term1 = 24.0f / (static_cast<float>(std::numbers::pi) * h8);
        float term2 = h2 - r2;
        float term3 = 4.0f * r2 - h2;

        return term1 * term2 * term3;
    }

    return 0.0f;
}

float FluidSimulation::spikyKernel3D(float r) const {
    if (r >= 0.0f && r <= h) {
        float x = h - r;
        return (15.0f / (std::numbers::pi * h6)) * x * x * x;
    }

    return 0.0f;
}

float FluidSimulation::spikyKernel2D(glm::vec2 dist) const {
    float r = std::abs(glm::length(dist));

    if (r <= h) {
        float x = h - r;
        return 10.0f / (std::numbers::pi * h5) * x * x * x;
    }

    return 0.0f;
}

glm::vec2 FluidSimulation::spikyKernelGradient2D(glm::vec2 dist) const {
    float r = glm::length(dist);

    if (r > 0 && r < h) {
        float x = h - r;
        float factor = (-30.0f / (std::numbers::pi * h5)) * x * x;
        return factor * (dist / r);
    }

    return glm::vec2(0.0f);
}

float FluidSimulation::viscosityKernel2D(float r) const {
    if (r >= 0.0f && r <= h) {
        return
            (15.0f / (2.0f * std::numbers::pi * h3))
        *   (   -(r * r * r) / (2.0f * h3)
                + (r * r) / h2
                + h / (2.0f * r)
                - 1
            );
    }

    return 0.0f;
}

float FluidSimulation::viscosityLaplacianKernel2D(glm::vec2 dist) const {
    float r = glm::length(dist);

    if (r <= h) {
        return (45.0f / (std::numbers::pi * h6)) * (h - r);
    }

    return 0.0f;
}

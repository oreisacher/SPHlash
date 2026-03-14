//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATION_H
#define FLUIDSIMSPH_FLUIDSIMULATION_H

#include <cmath>
#include <iostream>
#include <unordered_map>
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
    glm::vec2 surfaceTensionForce;
};

struct SpatialGrid {
    int hash(int x, int y) {
        return (x * 73856093) ^ (y * 19349663);
    }

    void insert(int id, glm::vec2 pos) {
        int cellX = std::floor(pos.x / cellSize);
        int cellY = std::floor(pos.y / cellSize);

        int h = hash(cellX, cellY);
        grid[h].push_back(id);
    }

    std::vector<int> getNeighbors(glm::vec2 pos) {
        int cellX = std::floor(pos.x / cellSize);
        int cellY = std::floor(pos.y / cellSize);

        std::vector<int> neighbors;

        for (int y = -1 ; y <= 1 ; y++) {
            for (int x = -1 ; x <= 1 ; x++) {
                int h = hash(cellX + x, cellY + y);

                auto it = grid.find(h);
                if (it == grid.end())
                    continue;

                for (int id : it->second)
                    neighbors.push_back(id);
            }
        }

        return neighbors;
    }

    void clear() {
        grid.clear();
    }

    std::unordered_map<int, std::vector<int>> grid;
    float cellSize;
};

class FluidSimulation {
public:
    FluidSimulation(int particleCount, float xMin, float yMin, float xMax, float yMax);

    void update(float deltaTime);

    int getParticleCount() const;
    const Particle* getData() const;

    void updateBoundaries(float width, float height);
    void updateMousePos(float x, float y);
    void updateMouseForceDirection(float direction);
    void updateMouseRadius(float radius);

    float getMouseRadius() const;

private:
    // Calculations
    void calculateDensityPressure();

    void calculateSPHForces();
    void calculatePressureForce();
    void calculateViscosityForce();
    void calculateSurfaceTensionForce();

    void applyMouseForce();

    void updateParticles(float deltaTime);

    // Kernels
    float poly6Kernel3D(float r) const;

    float poly6Kernel2D(glm::vec2 dist) const;
    glm::vec2 poly6KernelGradient2D(glm::vec2 dist) const;
    float poly6LaplacianKernel2D(glm::vec2 dist) const;

    float spikyKernel3D(float r) const;
    float spikyKernel2D(glm::vec2 dist) const;
    glm::vec2 spikyKernelGradient2D(glm::vec2 dist) const;

    float viscosityKernel2D(float r) const;
    float viscosityLaplacianKernel2D(glm::vec2 dist) const;

    SpatialGrid grid;
    std::vector<Particle> particles;

    // Boundaries
    float width = 1, height = 1;

    // Mouse
    float mouseX, mouseY;
    float mouseForceDirection = 0.0f;
    float mouseRadius = 0.1f;

    float h = 0.0f;
    float p_0 = 1000.0f;

    // Constants
    // float k = 2000.0f;
    // float mu = 100.0f;
    // float tension = 2.0f;

    // Without gravity
    float k = 0.7f;
    float mu = 0.3f;
    float tension = 0.000f;

    // With gravity
    // float k = 1.0f;
    // float mu = 0.1f;
    // float tension = 0.1f;

    // Cache
    float h2;
    float h3;
    float h5;
    float h6;
    float h8;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATION_H
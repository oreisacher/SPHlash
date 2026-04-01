//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include "glm/glm.hpp"
#include "rendering/Shader.h"
#include <cuda_runtime.h>

struct ParticleGPU {
    float4 position;     // vec4 to avoid padding issues
    float4 startPosition;
    float4 velocity;
    float4 properties;   // x: mass, y: density, z: pressure, w: id

    // SPH Forces
    float4 pressureForce;
    float4 viscosityForce;
    float4 surfaceTensionForce;

    // Debugging
    float4 debugging;
};

struct SimulationSettings {
    // Particles
    float h = 20.0f;
    float p_0 = 1000.0f;
    float k = 0.7f;
    float mu = 0.1f;
    float tensionThreshold = 8.0f;
    float tension = 0.001f;

    // Gravity
    bool enableGravity = false;
    float gravity = -9.878;

    // Boundary
    glm::vec2 xBoundary = { 0.0, 1.0 };
    glm::vec2 yBoundary = { 0.0, 1.0 };
    glm::vec2 zBoundary = { 0.0, 1.0 };
};

class FluidSimulationGPU {
public:
    FluidSimulationGPU(int particleCount, glm::vec2 x, glm::vec2 y, glm::vec2 z);
    ~FluidSimulationGPU();

    void update(float deltaTime);

    void reset() const;

    // void updateBoundaries();
    void setupVBOConnection(GLuint vbo);

    int particleCount;
    int hashTableSize;

    std::vector<ParticleGPU> particles;
    SimulationSettings settings;

private:
    void initParticlesRandom(glm::vec2 x, glm::vec2 y, glm::vec2 z);
    void initParticlesGrid(glm::vec2 x, glm::vec2 y, glm::vec2 z);

    ParticleGPU* d_particles            = nullptr;
    ParticleGPU* d_sortedParticles      = nullptr;

    // --- Spatial Hashing ---
    int* d_particleHashes               = nullptr;
    int* d_sortedParticleHashes         = nullptr;
    int* d_offsets                      = nullptr;

    // --- Common ---
    int* d_particleIndices              = nullptr;
    int* d_sortedParticleIndices        = nullptr;

    // --- Sorting Storage ---
    void* d_sortTempStorage             = nullptr;
    size_t sortTempStorageBytes         = 0;

    int gridSizeX = 1;
    int gridSizeY = 1;
    int gridSizeZ = 1;
    int totalCells = 1;

    cudaGraphicsResource* particleResource;

    // Mouse
    float mouseX, mouseY;
    float mouseForceDirection = 0.0f;
    float mouseRadius = 0.1f;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
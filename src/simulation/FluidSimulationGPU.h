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
    float4 properties;   // x: mass, y: density, z: pressure, w: unused

    // SPH Forces
    float4 pressureForce;
    float4 viscosityForce;
    float4 surfaceTensionForce;

    // Debugging
    float4 debugging;
};

struct HashEntry {
    uint32_t hash;
    uint32_t index;
};

struct SimulationSettings {
    float h = 20.0f;
    float p_0 = 1000.0f;
    float k = 0.7f;
    float mu = 0.1f;

    // 8 was rather good
    float tensionThreshold = 8.0f;
    float tension = 0.001f;

    bool enableGravity = false;
    float gravity = -9.878;
};

class FluidSimulationGPU {
public:
    FluidSimulationGPU(int particleCount, float xMin, float yMin, float xMax, float yMax);
    ~FluidSimulationGPU();

    void update(float deltaTime);

    void reset();

    void updateMousePos(float x, float y);
    void updateMouseForceDirection(float direction);
    void updateMouseRadius(float direction);

    void updateBoundaries(float width, float height);
    void setupVBOConnection(GLuint vbo);

    int particleCount;
    std::vector<ParticleGPU> particles;
    SimulationSettings settings;

private:
    // Fixed Grid
    int* d_particleCellIndex;
    int* d_particleIndex;
    int* d_cellStart;
    int* d_cellEnd;

    int gridSizeX = 1;
    int gridSizeY = 1;
    int totalCells = 1;

    cudaGraphicsResource* particleResource;

    // Boundaries
    float width = 1, height = 1;

    // Mouse
    float mouseX, mouseY;
    float mouseForceDirection = 0.0f;
    float mouseRadius = 0.1f;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
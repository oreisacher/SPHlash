//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include "glm/glm.hpp"
#include "rendering/Shader.h"
#include <cuda_runtime.h>

enum ParticleType {
    GHOST   = 0,
    NORMAL  = 1
};

enum SimulationState {
    UNINITIALIZED,
    PAUSED,
    RUNNING
};

struct ParticleGPU {
    float4 position;     // vec4 to avoid padding issues
    float4 startPosition;
    float4 velocity;
    float4 properties;   // x: mass, y: density, z: pressure, w: id
    float4 flags;        // x: type, y: undefined, z: undefined, w: undefined

    // SPH Forces
    float4 pressureForce;
    float4 viscosityForce;
    float4 surfaceTensionForce;

    // Debugging
    float4 debugging;
};

struct InitSettings {
    float totalMass = 1000.0f;
    float particleSpacing = 0.025f;

    // Spawn area
    glm::vec2 xBoundary = { 0.1, 0.9 };
    glm::vec2 yBoundary = { 0.1, 0.9 };
    glm::vec2 zBoundary = { 0.1, 0.9 };
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
    glm::vec2 xBoundary = { 0.0, 2.0 };
    glm::vec2 yBoundary = { 0.0, 5.0 };
    glm::vec2 zBoundary = { 0.0, 1.0 };
};

class FluidSimulationGPU {
public:
    FluidSimulationGPU();
    ~FluidSimulationGPU();

    void update(float deltaTime);
    void reset() const;

    void initParticles();

    // void updateBoundaries();
    void setRendererVBO(GLuint vbo);

    float cfl;

    int particleCount = 0;
    std::vector<ParticleGPU> particles;

    int boundaryParticleCount;
    std::vector<ParticleGPU> boundaryParticles;

    InitSettings initSettings;
    SimulationSettings settings;

    // --- Simulation State ---
    SimulationState state = UNINITIALIZED;
private:
    // --- Particles ---
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

    // --- Rendering ---
    cudaGraphicsResource* rendererResource = nullptr;
    float4* d_rendererParticles = nullptr;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
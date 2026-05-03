//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include "glm/glm.hpp"
#include "rendering/Shader.h"
#include <cuda_runtime.h>

#include "rendering/FluidRenderer.h"
#include "utils/AABB.h"
#include "utils/VoxelField.h"

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
    float4 position                 = { 0.0f, 0.0f, 0.0f, 0.0f };   // vec4 to avoid padding issues
    float4 startPosition            = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 velocity                 = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 properties               = { -1.0f, -1.0f, -1.0f, -1.0f };   // x: mass, y: density, z: pressure, w: id
    float4 flags                    = { 0.0f, 0.0f, 0.0f, 0.0f };   // x: type, y: undefined, z: undefined, w: undefined

    // SPH Forces
    float4 pressureForce            = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 viscosityForce           = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 surfaceTensionForce      = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Debugging
    float4 debugging                = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct InitSettings {
    float totalMass = 1000.0f;
    float particleSpacing = 0.025f;

    // Spawn area
    AABB spawnArea = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    };
};

struct SimulationSettings {
    // Domain
    AABB domainArea = {
        { 0.0f, 0.0f, 0.0f },
        { 2.0f, 2.0f, 2.0f }
    };

    // Particles
    float h = 0.05f;
    float p_0 = 1000.0f;
    float k = 0.7f;
    float mu = 0.1f;
    float tensionThreshold = 8.0f;
    float tension = 0.001f;

    // Gravity
    bool enableGravity = false;
    float gravity = -9.878;

    // CFL
    bool useCFL = true;
    float safetyFactor = 0.4f;
};

class FluidSimulation {
public:
    static FluidSimulation* getInstance();

    /*
     * Calculates density, pressure, etc. values for the current particles.
     * At the end d_particles contains the sorted particles!
     */
    void updateParticles();

    /*
     * Updates the simulation by the given timestep. First calculates density, pressure, etc.
     * with updateParticles(). Then calculates SPH specific forces. Using CFL, updates the positions
     * and velocities of each particle.
     */
    void update(float deltaTime);

    void step(float deltaTime);

    void start();
    void pause();
    void reset();

    void initParticles();

    void setRendererVBO(GLuint vbo);

    // --- VoxelField ---
    bool computeVoxelField(const VoxelField& field) const;

    ParticleGPU getParticle(int id) const;

    float cfl = 0;
    int particleCount = 0;

    InitSettings initSettings;
    SimulationSettings settings;

    // --- Simulation State ---
    SimulationState state = UNINITIALIZED;
private:
    FluidSimulation();
    ~FluidSimulation();

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
    RendererParticle* d_rendererParticles = nullptr;

    void calculateDensityMap();
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
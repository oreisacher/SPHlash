//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include "glm/glm.hpp"
#include <cuda_runtime.h>
#include "rendering/BoundingBoxNode.h"
#include "rendering/FluidRenderer.h"
#include "utils/AABB.h"
#include "utils/Hex32Field.h"
#include "utils/VoxelField.h"

enum SimulationState {
    UNINITIALIZED,
    PAUSED,
    RUNNING
};

struct ParticleGPU {
    float4 position                 = { 0.0f, 0.0f, 0.0f, 0.0f };       // vec4 to avoid padding issues
    float4 startPosition            = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 velocity                 = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 properties               = { -1.0f, -1.0f, -1.0f, -1.0f };   // x: mass, y: density, z: pressure, w: id
    float4 flags                    = { 0.0f, 0.0f, 0.0f, 0.0f };       // x: surface, y: undefined, z: undefined, w: undefined
    float4 color                    = { 0.0f, 0.0f, 0.0f, 1.0f };

    // SPH Forces
    float4 pressureForce            = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 viscosityForce           = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 surfaceTensionForce      = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Debugging
    float4 debugging                = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct CudaForceField {
    float3 position     = { 0.0f, 0.0f, 0.0f };
    float3 halfExtents  = { 0.2f, 0.2f, 0.2f };

    float3 right        = { 1.0f, 0.0f, 0.0f };
    float3 up           = { 0.0f, 1.0f, 0.0f };
    float3 forward      = { 0.0f, 0.0f, 1.0f };

    bool active         = true;
    float strength      = 5.0f;
};

struct CudaParticleSpawn {
    float restDensity       = 1000.0f;
    float particleSpacing   = 0.025f;
    float4 color            = { 1.0f, 0.0f, 0.0f, 1.0f };

    CudaAABB area = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    };

    bool hasShape               = false;
    Triangle* d_shapeTriangles  = nullptr;
};

struct SimulationSettings {
    // Particles
    float h = 0.05f;
    float p_0 = 1000.0f;
    float maxVelocity = 6.0f;
    float speedSound = 10.0f;
    float alpha = 0.05f;
    float surfaceTension = 0.22f;

    // Gravity
    bool enableGravity = true;
    float gravity = -9.878;

    // CFL
    bool useCFL = true;
    float safetyFactor = 0.25f;
};

class FluidSimulation {
public:
    FluidSimulation(Scene& scene);
    ~FluidSimulation();

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

    void init();
    void start();
    void pause();
    void reset();
    SimulationState getState() const;

    bool initParticles();

    void setRendererVBO(GLuint vbo);

    float getCFL() const;
    int getParticleCount() const;

    BoundingBoxNode* getDomain() const;

    void rebuildForceFieldBuffer();
    void updateForceFieldBuffer() const;

    void setBoundaryMaps(const Hex32Field &sdfMap, const Hex32Field &volumeMap);
    void clearBoundaryField();
    Hex32Field getBoundaryField() const;

    // --- VoxelField ---
    bool computeVoxelField(const VoxelField& field) const;

    ParticleGPU getParticle(int id) const;

    SimulationSettings settings;

    inline static const char* SimulationDomainTag = "simulation_domain";

private:
    Scene* scene;
    BoundingBoxNode* domain;

    float cfl = 0;
    int particleCount = 0;

    // --- Simulation State ---
    SimulationState state = UNINITIALIZED;

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

    // --- Force Fields ---
    CudaForceField* d_forceFields           = nullptr;

    // --- Boundary Density Map ---
    Hex32Field sdfMap;
    Hex32Field volumeMap;

    // --- Rendering ---
    cudaGraphicsResource* rendererResource = nullptr;
    RendererParticle* d_rendererParticles  = nullptr;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

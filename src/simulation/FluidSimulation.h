//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include <vector_types.h>
#include "boundaries/Hex32Field.h"
#include <cstddef>
#include <vector>

struct ParticlesGPU;
class ParticlesNode;
class Scene;
struct cudaGraphicsResource;
struct VoxelField;
class BoundingBoxNode;
struct RendererParticle;
struct CudaForceField;

enum SimulationState {
     UNINITIALIZED,
     PAUSED,
     RUNNING
};

/*
    Represents a single particle inside simulation. Can be queried via getParticle(id). Host only
*/
struct ParticleHost {
    float4 position;
    float4 startPosition;
    float4 velocity;
    float4 properties;
    float4 flags;
    float4 color;
    float4 pressureForce;
    float4 viscosityForce;
    float4 surfaceTensionForce;
};

struct ParticlesHost {
    std::vector<float4> position;
    std::vector<float4> startPosition;
    std::vector<float4> velocity;
    std::vector<float4> properties;
    std::vector<float4> flags;
    std::vector<float4> color;
    std::vector<float4> pressureForce;
    std::vector<float4> viscosityForce;
    std::vector<float4> surfaceTensionForce;

    void resize(size_t count);
    void append(ParticlesHost& particles);
    void copyToDevice(const ParticlesGPU& device, size_t count) const;
};

/*
    Represents all particles of simulation as SoA. Contains device pointers. Used during simulation.
*/
struct ParticlesGPU {
    float4* d_position              = nullptr;
    float4* d_startPosition         = nullptr;
    float4* d_velocity              = nullptr;
    float4* d_properties            = nullptr;  //  x: mass,     y: density,     z: pressure,    w: id
    float4* d_flags                 = nullptr;  //  x: surface,  y: undefined,   z: undefined,   w: undefined
    float4* d_color                 = nullptr;
    float4* d_pressureForce         = nullptr;
    float4* d_viscosityForce        = nullptr;
    float4* d_surfaceTensionForce   = nullptr;

    void allocate(size_t count);
    void free();
    void copyToHost(ParticlesHost& host, size_t count) const;
    void copyToHost(ParticleHost& host, int id) const;
};

struct SimulationSettings {
    // Simulation
    int simulationSteps = 1;

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

    // Time
    bool useFixedTimeStep = false;
    float fixedTimeStep = 0.5f;

    // CFL
    bool useCFL = true;
    float safetyFactor = 0.25f;
};

class FluidSimulation {
public:
    FluidSimulation(Scene* scene, bool headless = false);
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

    void setRendererVBO(unsigned int vbo);

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

    ParticleHost getParticle(int id) const;

    SimulationSettings settings;

    inline static const char* SimulationDomainTag = "simulation_domain";

private:
    Scene* scene;
    BoundingBoxNode* domain;

    float cfl = 0;
    int particleCount = 0;
    bool headless = false;

    // --- Simulation State ---
    SimulationState state = UNINITIALIZED;

    // --- Particles ---
    ParticlesGPU particles;
    ParticlesGPU sortedParticles;

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
    ParticlesNode* particlesNode;
    cudaGraphicsResource* rendererResource = nullptr;
    RendererParticle* d_rendererParticles  = nullptr;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

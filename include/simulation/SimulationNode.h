//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include "ParticleHost.h"
#include "ParticlesGPU.h"
#include "boundaries/Hex32Field.h"
#include "scene/nodes/SceneNode.h"

#define THREADS_PER_BLOCK 256
#define BLOCK_COUNT(particleCount) (particleCount + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK

struct ParticlesGPU;
class ParticlesNode;
class Scene;
struct cudaGraphicsResource;
struct VoxelField;
class BoundingBoxNode;
struct RendererParticle;
struct ForceField;

enum SimulationState {
     UNINITIALIZED,
     PAUSED,
     RUNNING
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

class SimulationNode : public SceneNode {
public:
    SimulationNode();
    ~SimulationNode() override;

    void onAddedToScene() override;

    void initSimulation();

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
    void update(float deltaTime) override;

    void step(float deltaTime);

    void start();
    void pause();
    void reset();
    SimulationState getState() const;

    bool initParticles();

    void setRendererVBO(unsigned int vbo);

    float getCFL() const;
    int getParticleCount() const;

    void rebuildForceFieldBuffer();
    void updateForceFieldBuffer() const;

    void setBoundaryMaps(const Hex32Field &sdfMap, const Hex32Field &volumeMap);
    void clearBoundaryField();
    Hex32Field getBoundaryField() const;

    // --- VoxelField ---
    bool computeVoxelField(const VoxelField& field) const;

    ParticleHost getParticle(int id) const;
    ParticlesGPU getParticlesGPU() const;
    std::string getTypeName() const override;

    SimulationSettings settings;

    inline static const char* SimulationDomainTag = "simulation_domain";

private:
    int particleCount = 0;

    float cfl = 0;

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
    ForceField* d_forceFields           = nullptr;

    // --- Boundary Density Map ---
    Hex32Field sdfMap;
    Hex32Field volumeMap;

    // --- Rendering ---
    ParticlesNode* particlesNode           = nullptr;
    cudaGraphicsResource* rendererResource = nullptr;
    RendererParticle* d_rendererParticles  = nullptr;
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

//
// Created by ruut on 3/16/26.
//

#include "FluidSimulation.h"
#include <algorithm>
#include <cuda_runtime.h>
#include <iostream>
#include <random>
#include <nvtx3/nvToolsExt.h>
#include "cuda/Boundaries.h"
#include "cuda/FluidSimulation.h"
#include <cuda_gl_interop.h>
#include <thrust/system/cuda/detail/par.h>

#include "imgui.h"
#include "rendering/BoundingBoxNode.h"
#include "rendering/ForceFieldNode.h"
#include "rendering/Scene.h"
#include "utils/CudaUtils.h"

FluidSimulation::FluidSimulation(Scene* scene, bool headless) : scene(scene), headless(headless) {
    cudaSetDevice(0);

    // Setup domain bounding box
    auto domainBB = std::make_unique<BoundingBoxNode>("Simulation Domain", glm::vec4(0.2f, 0.6f, 1.0f, 1.0f));
    domain = domainBB.get();
    domain->setScale({ 2.0f, 2.0f, 2.0f });
    domain->setDeletable(false);
    domain->addTag(SimulationDomainTag);
    scene->setSimulationDomain(domain);
    scene->addSceneNode(std::move(domainBB));

    // Setup particles node for rendering
    if (!headless) {
        auto particlesNode = std::make_unique<ParticlesNode>(this);
        particlesNode->addTag("fluid");
        particles = particlesNode.get();
        scene->setParticleNode(particles);
        scene->addSceneNode(std::move(particlesNode));
    }
}

FluidSimulation::~FluidSimulation() {
    cudaFree(d_particles);
    cudaFree(d_sortedParticles);
    cudaFree(d_particleHashes);
    cudaFree(d_sortedParticleHashes);
    cudaFree(d_offsets);
    cudaFree(d_particleIndices);
    cudaFree(d_sortedParticleIndices);
    cudaFree(d_sortTempStorage);
    cudaFree(d_forceFields);

    if (!headless)
        cudaGraphicsUnmapResources(1, &rendererResource, nullptr);
}

void FluidSimulation::updateParticles() {
    if (state == UNINITIALIZED)
        return;

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    // Reset old data
    cudaMemset(d_offsets,   -1, particleCount * sizeof(int));

    launchHashParticlesKernel(
        particleCount,
        d_particles,
        d_particleHashes,
        d_particleIndices,
        settings.h
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("HashParticlesKernel");
#endif

    sortParticlesByHash(
        particleCount,
        d_sortTempStorage,
        sortTempStorageBytes,
        d_particleHashes,
        d_sortedParticleHashes,
        d_particleIndices,
        d_sortedParticleIndices
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("SortParticlesByHash");
#endif

    launchBuildOffsetsKernel(
        particleCount,
        d_sortedParticleHashes,
        d_offsets
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("BuildOffsetsKernel");
#endif

    launchReorderParticlesKernel(
        particleCount,
        d_particles,
        d_sortedParticleIndices,
        d_sortedParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("ReorderParticlesKernel");
#endif

    launchDensityPressureKernel(
        particleCount,
        d_sortedParticles,
        d_sortedParticleHashes,
        d_offsets,
        sdfMap.d_cells != nullptr,
        sdfMap,
        volumeMap,
        domain->getCudaAABB(),
        settings.h,
        settings.speedSound,
        settings.p_0
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("DensityPressureKernel");
#endif

    launchSphForcesKernel(
        particleCount,
        d_sortedParticles,
        d_sortedParticleHashes,
        d_offsets,
        sdfMap.d_cells != nullptr,
        sdfMap,
        volumeMap,
        domain->getCudaAABB(),
        settings.h,
        settings.alpha,
        settings.speedSound,
        settings.surfaceTension,
        settings.p_0
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("SphForcesKernel");
#endif

    std::swap(d_particles, d_sortedParticles);
}

void FluidSimulation::update(const float deltaTime) {
    if (state == UNINITIALIZED  || state == PAUSED)
        return;

    updateForceFieldBuffer();
    updateParticles();

    // --- CFL ---
    float cs = 10.0f * settings.maxVelocity;
    settings.speedSound = cs;

    float cflTimeStep = settings.safetyFactor * settings.h / (cs * (1.0f + 0.6f * settings.alpha));

    float maxAcc = getMaxAcceleration(particleCount, d_particles);

    if (settings.enableGravity)
        maxAcc = fminf(maxAcc, abs(settings.gravity));

    float accTimeStep = 0.25f * sqrtf(settings.h / maxAcc);

    cfl = fminf(cflTimeStep, accTimeStep);

    launchUpdateParticlesKernel(
        d_particles,
        particleCount,
        d_forceFields,
        scene->getForceFields().size(),
        settings.useCFL ? fminf(cfl, deltaTime) : deltaTime,
        domain->getCudaAABB(),
        settings.enableGravity ? settings.gravity : 0.0f
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("UpdateParticlesKernel");
#endif

    if (!headless) {
        // Prepare particles for rendering
        size_t numBytes;
        cudaGraphicsMapResources(1, &rendererResource);
        cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_rendererParticles), &numBytes, rendererResource);

        launchRenderPrepareKernel(
            particleCount,
            d_particles,
            d_rendererParticles
        );

#ifdef DEBUG_CUDA
        CUDA_KERNEL_CHECK("RenderPrepareKernel");
#endif

        cudaGraphicsUnmapResources(1, &rendererResource);
    }
}

void FluidSimulation::step(float deltaTime) {
    if (state == UNINITIALIZED)
        return;

    state = RUNNING;
    update(deltaTime);
    state = PAUSED;
}

void FluidSimulation::init() {
    // Init particle spawns
    bool success = initParticles();

    if (!headless) {
        if (!success) {
            particles->setVisible(false);
            return;
        }

        particles->setVisible(true);
        particles->updateParticlesSize(particleCount);

        // Update particle rendering
        setRendererVBO(particles->getParticleVBO());
    }

    // Rebuild forcefield buffer
    rebuildForceFieldBuffer();
}

void FluidSimulation::start() {
    state = RUNNING;
}

void FluidSimulation::pause() {
    state = PAUSED;
}

void FluidSimulation::reset() {
    state = PAUSED;

    launchResetParticlesKernel(d_particles, particleCount);

    updateParticles();

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    // Copy reset data to renderer vbo
    launchRenderPrepareKernel(
        particleCount,
        d_particles,
        d_rendererParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("RenderPrepareKernel");
#endif
}

SimulationState FluidSimulation::getState() const {
    return state;
}

void FluidSimulation::setRendererVBO(const GLuint vbo) {
    if (rendererResource) cudaGraphicsUnregisterResource(rendererResource);

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(
        &rendererResource,
        vbo,
        cudaGraphicsRegisterFlagsWriteDiscard
    ));

    size_t numBytes;
    CUDA_CHECK(cudaGraphicsMapResources(1, &rendererResource));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_rendererParticles), &numBytes, rendererResource));

    launchRenderPrepareKernel(
        particleCount,
        d_particles,
        d_rendererParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("RenderPrepareKernel");
#endif

    CUDA_CHECK(cudaGraphicsUnmapResources(1, &rendererResource));
}

float FluidSimulation::getCFL() const {
    return cfl;
}

int FluidSimulation::getParticleCount() const {
    return particleCount;
}

BoundingBoxNode* FluidSimulation::getDomain() const {
    return domain;
}

void FluidSimulation::rebuildForceFieldBuffer() {
    if (d_forceFields) cudaFree(d_forceFields);

    if (scene->getForceFields().size() == 0)
        return;

    cudaMalloc(&d_forceFields, scene->getForceFields().size() * sizeof(CudaForceField));
}

void FluidSimulation::updateForceFieldBuffer() const {
    std::vector<CudaForceField> forceFields;
    forceFields.reserve(scene->getForceFields().size());

    for (auto& forceField : scene->getForceFields())
        forceFields.push_back(forceField->getForceField());

    cudaMemcpy(d_forceFields, forceFields.data(), forceFields.size() * sizeof(CudaForceField), cudaMemcpyHostToDevice);
}

void FluidSimulation::setBoundaryMaps(const Hex32Field& sdfMap, const Hex32Field& volumeMap) {
    this->sdfMap = sdfMap;
    this->volumeMap = volumeMap;
}

void FluidSimulation::clearBoundaryField() {
    if (sdfMap.d_cells != nullptr) {
        cudaFree(sdfMap.d_cells);
        sdfMap.d_cells = nullptr;
        sdfMap.resolution = { 0, 0, 0 };
    }

    if (volumeMap.d_cells != nullptr) {
        cudaFree(volumeMap.d_cells);
        volumeMap.d_cells = nullptr;
        volumeMap.resolution = { 0, 0, 0 };
    }
}

Hex32Field FluidSimulation::getBoundaryField() const {
    return volumeMap;
}

bool FluidSimulation::computeVoxelField(const VoxelField& field) const {
    if (particleCount == 0 || state == UNINITIALIZED)
        return false;

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    launchVoxelFieldKernel(
        field,
        domain->getCudaAABB(),
        particleCount,
        d_particles,
        d_sortedParticleHashes,
        d_offsets,
        settings.h
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("VoxelFieldKernel");
#endif

    return true;
}

ParticleGPU FluidSimulation::getParticle(const int id) const {
    ParticleGPU particle;

    if (state == UNINITIALIZED)
        return particle;

    if (id < 0 || id >= particleCount)
        return particle;

    cudaMemcpy(&particle, &d_particles[id], sizeof(ParticleGPU), cudaMemcpyDeviceToHost);
    return particle;
}

bool FluidSimulation::initParticles() {
#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    // Delete old particle data
    particleCount = 0;

    if (d_particleHashes)           cudaFree(d_particleHashes);         d_particleHashes = nullptr;
    if (d_sortedParticleHashes)     cudaFree(d_sortedParticleHashes);   d_sortedParticleHashes = nullptr;
    if (d_particles)                cudaFree(d_particles);              d_particles = nullptr;
    if (d_sortedParticles)          cudaFree(d_sortedParticles);        d_sortedParticles = nullptr;
    if (d_particleIndices)          cudaFree(d_particleIndices);        d_particleIndices = nullptr;
    if (d_sortedParticleIndices)    cudaFree(d_sortedParticleIndices);  d_sortedParticleIndices = nullptr;
    if (d_offsets)                  cudaFree(d_offsets);                d_offsets = nullptr;
    if (d_sortTempStorage)          cudaFree(d_sortTempStorage);        d_sortTempStorage = nullptr;

    std::vector<ParticleGPU> allParticles;

    for (const auto& spawn : scene->getParticleSpawns()) {
        CudaParticleSpawn cudaSpawn = spawn->getParticleSpawn();

        int countX          = std::floor((cudaSpawn.area.max.x - cudaSpawn.area.min.x) / cudaSpawn.particleSpacing + 1e-5f) + 1;
        int countY          = std::floor((cudaSpawn.area.max.y - cudaSpawn.area.min.y) / cudaSpawn.particleSpacing + 1e-5f) + 1;
        int countZ          = std::floor((cudaSpawn.area.max.z - cudaSpawn.area.min.z) / cudaSpawn.particleSpacing + 1e-5f) + 1;

        int maxSpawnParticles = countX * countY * countZ;

        ParticleGPU* d_spawnParticles;
        cudaMalloc(reinterpret_cast<void**>(&d_spawnParticles), maxSpawnParticles * sizeof(ParticleGPU));

        int* d_spawnParticleCount;
        cudaMalloc(&d_spawnParticleCount, sizeof(int));
        cudaMemset(d_spawnParticleCount, 0, sizeof(int));

        launchInitParticlesKernel(
            cudaSpawn,
            d_spawnParticles,
            d_spawnParticleCount,
            { countX, countY, countZ }
        );

#ifdef DEBUG_CUDA
        CUDA_KERNEL_CHECK("InitParticlesKernel");
#endif

        cudaDeviceSynchronize();

        int spawnParticleCount;
        cudaMemcpy(&spawnParticleCount, d_spawnParticleCount, sizeof(int), cudaMemcpyDeviceToHost);
        cudaFree(d_spawnParticleCount);

        float volume;
        if (cudaSpawn.hasShape) {

        } else {
            volume = (cudaSpawn.area.max.x - cudaSpawn.area.min.x) *
                     (cudaSpawn.area.max.y - cudaSpawn.area.min.y) *
                     (cudaSpawn.area.max.z - cudaSpawn.area.min.z);
        }

        float mass = (cudaSpawn.restDensity * volume) / spawnParticleCount;

        launchUpdateParticleMassKernel(
            spawnParticleCount,
            d_spawnParticles,
            mass
        );

#ifdef DEBUG_CUDA
        CUDA_KERNEL_CHECK("UpdateParticlesMassKernel");
#endif

        cudaDeviceSynchronize();

        // Copy spawn particles back to host and append
        std::vector<ParticleGPU> spawnParticles(spawnParticleCount);
        cudaMemcpy(spawnParticles.data(), d_spawnParticles, spawnParticleCount * sizeof(ParticleGPU), cudaMemcpyDeviceToHost);
        cudaFree(d_spawnParticles);

        allParticles.insert(allParticles.end(), spawnParticles.begin(), spawnParticles.end());
    }

    particleCount = allParticles.size();

    if (particleCount == 0) {
        state = UNINITIALIZED;
        return false;
    }

    // Allocate memory on gpu
    cudaMalloc(reinterpret_cast<void**>(&d_particles), particleCount * sizeof(ParticleGPU));
    cudaMalloc(reinterpret_cast<void**>(&d_sortedParticles), particleCount * sizeof(ParticleGPU));
    cudaMalloc(reinterpret_cast<void**>(&d_particleHashes), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_sortedParticleHashes), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleIndices), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_sortedParticleIndices), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_offsets), particleCount * sizeof(int));

    // Allocate memory for CUB sorting
    queryRequiredSortingStorage(
        particleCount,
        sortTempStorageBytes,
        d_particleHashes,
        d_sortedParticleHashes,
        d_particleIndices,
        d_sortedParticleIndices
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("QueryRequiredSortingStorage");
#endif

    cudaMalloc(&d_sortTempStorage, sortTempStorageBytes);

    cudaMemcpy(d_particles, allParticles.data(), particleCount * sizeof(ParticleGPU), cudaMemcpyHostToDevice);
    cudaMemcpy(d_sortedParticles, allParticles.data(), particleCount * sizeof(ParticleGPU), cudaMemcpyHostToDevice);

    cudaDeviceSynchronize();

    state = PAUSED;
    updateParticles();

    return true;
}
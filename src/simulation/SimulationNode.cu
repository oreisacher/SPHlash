//
// Created by ruut on 3/16/26.
//

#include "simulation/SimulationNode.h"
#include <algorithm>
#include <cuda_runtime.h>
#include <iostream>
#include <random>
#include <nvtx3/nvToolsExt.h>
#include "simulation/scene/nodes/BoundingBoxNode.h"
#include "simulation/scene/nodes/ForceFieldNode.h"
#include "simulation/scene/Scene.h"
#include "utils/CudaUtils.h"
#include <cuda_gl_interop.h>
#include <cub/device/device_radix_sort.cuh>

#include "core/Input.h"
#include "glm/ext/scalar_integer.hpp"
#include "simulation/Kernels.cuh"
#include "simulation/Particles.cuh"
#include "simulation/ParticlesHost.h"
#include "simulation/VoxelField.cuh"
#include "simulation/scene/IParticleSpawn.h"
#include "simulation/scene/nodes/ParticlesNode.h"
#include "simulation/scene/nodes/SimulationDomainNode.h"

SimulationNode::SimulationNode() {
    this->name = "Simulation";

    sceneGraphVisible = false;
    deletable = false;
    savable = false;

    cudaSetDevice(0);
}

SimulationNode::~SimulationNode() {
    particles.free();
    sortedParticles.free();

    cudaFree(d_particleHashes);
    cudaFree(d_sortedParticleHashes);
    cudaFree(d_offsets);
    cudaFree(d_particleIndices);
    cudaFree(d_sortedParticleIndices);
    cudaFree(d_sortTempStorage);
    cudaFree(d_forceFields);

    cudaGraphicsUnmapResources(1, &rendererResource, nullptr);
}

void SimulationNode::onAddedToScene() {
    // --- Simulation ---
    auto simulationDomain = std::make_unique<SimulationDomainNode>();
    scene->addSceneNode(std::move(simulationDomain));

    // --- Particles ---
    auto particlesNode = std::make_unique<ParticlesNode>(this);
    this->particlesNode = particlesNode.get();
    scene->addSceneNode(std::move(particlesNode));
}

void SimulationNode::updateParticles() {
    if (state == UNINITIALIZED)
        return;

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    // Reset old data
    cudaMemset(d_offsets,   -1, tableSize * sizeof(int));

    hashParticlesKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        particles.d_position,
        d_particleHashes,
        d_particleIndices,
        settings.h,
        settings.kernelSettings,
        tableSize
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("HashParticlesKernel");
#endif

    cub::DeviceRadixSort::SortPairs(
        d_sortTempStorage,
        sortTempStorageBytes,
        d_particleHashes, d_sortedParticleHashes,
        d_particleIndices, d_sortedParticleIndices, particleCount
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("SortParticlesByHash");
#endif

    buildOffsetsKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        d_sortedParticleHashes,
        d_offsets
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("BuildOffsetsKernel");
#endif

    reorderParticlesKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        particles,
        d_sortedParticleIndices,
        sortedParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("ReorderParticlesKernel");
#endif

    densityPressureKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        sortedParticles.d_position,
        sortedParticles.d_properties,
        d_sortedParticleHashes,
        tableSize,
        d_offsets,
        sdfMap.d_cells != nullptr,
        sdfMap,
        volumeMap,
        scene->getSimulationDomain()->getWorldBoundingBox().getAABBView(),
        settings.h,
        settings.kernelSettings,
        settings.speedSound,
        settings.p_0
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("DensityPressureKernel");
#endif

    topSurfaceParticlesKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        sortedParticles.d_position,
        sortedParticles.d_flags,
        d_sortedParticleHashes,
        tableSize,
        d_offsets,
        settings.h,
        settings.kernelSettings
    );

    sphForcesKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        sortedParticles.d_position,
        sortedParticles.d_properties,
        sortedParticles.d_velocity,
        sortedParticles.d_pressureForce,
        sortedParticles.d_viscosityForce,
        sortedParticles.d_surfaceTensionForce,
        sortedParticles.d_flags,
        d_sortedParticleHashes,
        tableSize,
        d_offsets,
        sdfMap.d_cells != nullptr,
        sdfMap,
        volumeMap,
        scene->getSimulationDomain()->getWorldBoundingBox().getAABBView(),
        settings.h,
        settings.kernelSettings,
        settings.alpha,
        settings.speedSound,
        settings.surfaceTension,
        settings.p_0
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("SphForcesKernel");
#endif

    std::swap(particles.d_position, sortedParticles.d_position);
    std::swap(particles.d_startPosition, sortedParticles.d_startPosition);
    std::swap(particles.d_velocity, sortedParticles.d_velocity);
    std::swap(particles.d_properties, sortedParticles.d_properties);
    std::swap(particles.d_flags, sortedParticles.d_flags);
    std::swap(particles.d_color, sortedParticles.d_color);
    std::swap(particles.d_pressureForce, sortedParticles.d_pressureForce);
    std::swap(particles.d_viscosityForce, sortedParticles.d_viscosityForce);
    std::swap(particles.d_surfaceTensionForce, sortedParticles.d_surfaceTensionForce);
}

void SimulationNode::update(const float deltaTime) {
    if (state == UNINITIALIZED)
        return;

    if (state == PAUSED && !Input::Get()->isKeyPressed(Window::Key::P))
        return;

    updateForceFieldBuffer();

    // --- Delta Time ---
    float dt = settings.useFixedTimeStep ? settings.fixedTimeStep / 1000.0f : deltaTime;

    // --- Max external Force ---
    float maxExternalForce = 0;

    for (auto ffn : scene->getForceFields()) {
        auto ff = ffn->getForceField();

        if (!ff.active)
            continue;

        maxExternalForce = fmaxf(maxExternalForce, ff.strength);
    }

    maxExternalForce = fmaxf(maxExternalForce, fabsf(settings.gravity));

    // --- CFL ---
    float cs = fmaxf(10.0f * settings.maxVelocity, 0.1f);
    settings.speedSound = cs;

    float cflTimeStep = settings.safetyFactor * (settings.h / (cs * (1.0f + 0.6f * settings.alpha)));
    float accTimeStep = 0.25f * (settings.h / maxExternalForce);
    cfl = fminf(cflTimeStep, accTimeStep);

    for (int i = 0 ; i < settings.simulationSteps ; i++) {
        updateParticles();

        updateParticlesKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
            particles.d_pressureForce,
            particles.d_viscosityForce,
            particles.d_surfaceTensionForce,
            particles.d_velocity,
            particles.d_position,
            particleCount,
            d_forceFields,
            scene->getForceFields().size(),
            settings.useCFL ? fminf(cfl, dt) : dt,
            scene->getSimulationDomain()->getWorldBoundingBox().getAABBView(),
            settings.enableGravity ? settings.gravity : 0.0f
        );
    }

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("UpdateParticlesKernel");
#endif

    // Prepare particles for rendering
    size_t numBytes;
    cudaGraphicsMapResources(1, &rendererResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_rendererParticles), &numBytes, rendererResource);

    renderPrepareKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        particles.d_position,
        particles.d_velocity,
        particles.d_properties,
        particles.d_flags,
        particles.d_color,
        d_rendererParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("RenderPrepareKernel");
#endif

    cudaGraphicsUnmapResources(1, &rendererResource);
}

void SimulationNode::step(float deltaTime) {
    if (state == UNINITIALIZED)
        return;

    state = RUNNING;
    update(deltaTime);
    state = PAUSED;
}

void SimulationNode::initSimulation() {
    // Init particle spawns
    bool success = initParticles();

    if (!success) {
        particlesNode->setVisible(false);
        return;
    }

    particlesNode->setVisible(true);
    particlesNode->updateParticlesSize(particleCount);

    // Update particle rendering
    setRendererVBO(particlesNode->getParticleVBO());

    // Rebuild forcefield buffer
    rebuildForceFieldBuffer();
}

void SimulationNode::start() {
    state = RUNNING;
}

void SimulationNode::pause() {
    state = PAUSED;
}

void SimulationNode::reset() {
    state = PAUSED;

    resetParticlesKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particles.d_position,
        particles.d_startPosition,
        particles.d_velocity,
        particles.d_properties,
        particles.d_pressureForce,
        particles.d_viscosityForce,
        particles.d_surfaceTensionForce,
        particleCount
    );

    updateParticles();

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    // Copy reset data to renderer vbo
    renderPrepareKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        particles.d_position,
        particles.d_velocity,
        particles.d_properties,
        particles.d_flags,
        particles.d_color,
        d_rendererParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("RenderPrepareKernel");
#endif
}

SimulationState SimulationNode::getState() const {
    return state;
}

void SimulationNode::setRendererVBO(const GLuint vbo) {
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

    renderPrepareKernel<<<BLOCK_COUNT(particleCount), THREADS_PER_BLOCK>>>(
        particleCount,
        particles.d_position,
        particles.d_velocity,
        particles.d_properties,
        particles.d_flags,
        particles.d_color,
        d_rendererParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("RenderPrepareKernel");
#endif

    CUDA_CHECK(cudaGraphicsUnmapResources(1, &rendererResource));
}

float SimulationNode::getCFL() const {
    return cfl;
}

int SimulationNode::getParticleCount() const {
    return particleCount;
}

void SimulationNode::rebuildForceFieldBuffer() {
    if (d_forceFields) cudaFree(d_forceFields);

    if (scene->getForceFields().size() == 0)
        return;

    cudaMalloc(&d_forceFields, scene->getForceFields().size() * sizeof(ForceField));
}

void SimulationNode::updateForceFieldBuffer() const {
    std::vector<ForceField> forceFields;
    forceFields.reserve(scene->getForceFields().size());

    for (auto& forceField : scene->getForceFields())
        forceFields.push_back(forceField->getForceField());

    cudaMemcpy(d_forceFields, forceFields.data(), forceFields.size() * sizeof(ForceField), cudaMemcpyHostToDevice);
}

void SimulationNode::setBoundaryMaps(const Hex32Field& sdfMap, const Hex32Field& volumeMap) {
    this->sdfMap = sdfMap;
    this->volumeMap = volumeMap;
}

void SimulationNode::clearBoundaryField() {
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

Hex32Field SimulationNode::getBoundaryField() const {
    return volumeMap;
}

bool SimulationNode::computeVoxelField(const VoxelField& field) const {
    if (particleCount == 0 || state == UNINITIALIZED)
        return false;

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    int cells = field.resolution.x * field.resolution.y * field.resolution.z;
    voxelFieldKernel<<<BLOCK_COUNT(cells), THREADS_PER_BLOCK>>>(
        field,
        scene->getSimulationDomain()->getWorldBoundingBox().getAABBView(),
        particleCount,
        particles.d_position,
        particles.d_properties,
        d_sortedParticleHashes,
        tableSize,
        d_offsets,
        settings.h,
        settings.kernelSettings
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("VoxelFieldKernel");
#endif

    return true;
}

ParticleHost SimulationNode::getParticle(const int id) const {
    if (state == UNINITIALIZED)
        return {};

    if (id < 0 || id >= particleCount)
        return {};

    ParticleHost p{};
    particles.copyToHost(p, id);
    return p;
}

ParticlesGPU SimulationNode::getParticlesGPU() const {
    return particles;
}

std::string SimulationNode::getTypeName() const {
    return "SimulationNode";
}

bool SimulationNode::initParticles() {
#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    particleCount = 0;

    // Free old resources
    if (particles.d_position)       particles.free();
    if (sortedParticles.d_position) sortedParticles.free();
    if (d_particleHashes)           cudaFree(d_particleHashes);         d_particleHashes = nullptr;
    if (d_sortedParticleHashes)     cudaFree(d_sortedParticleHashes);   d_sortedParticleHashes = nullptr;
    if (d_particleIndices)          cudaFree(d_particleIndices);        d_particleIndices = nullptr;
    if (d_sortedParticleIndices)    cudaFree(d_sortedParticleIndices);  d_sortedParticleIndices = nullptr;
    if (d_offsets)                  cudaFree(d_offsets);                d_offsets = nullptr;
    if (d_sortTempStorage)          cudaFree(d_sortTempStorage);        d_sortTempStorage = nullptr;

    ParticlesHost allParticles;

    for (const auto& spawn : scene->getParticleSpawns()) {
        ParticleSpawn cudaSpawn = spawn->getParticleSpawn();

        glm::ivec3 count = glm::floor(cudaSpawn.area.size / cudaSpawn.particleSpacing + 1e-5f);
        int maxSpawnParticles = count.x * count.y * count.z;

        ParticlesGPU spawnParticles;
        spawnParticles.allocate(maxSpawnParticles);

        int* d_spawnParticleCount;
        cudaMalloc(&d_spawnParticleCount, sizeof(int));
        cudaMemset(d_spawnParticleCount, 0, sizeof(int));

        initParticlesKernel<<<BLOCK_COUNT(maxSpawnParticles), THREADS_PER_BLOCK>>>(
            cudaSpawn,
            spawnParticles,
            d_spawnParticleCount,
            sdfMap.d_cells != nullptr,
            sdfMap,
            scene->getSimulationDomain()->getWorldBoundingBox().getAABBView(),
            count,
            settings.h
        );

#ifdef DEBUG_CUDA
        CUDA_KERNEL_CHECK("InitParticlesKernel");
#endif

        int spawnParticleCount;
        cudaMemcpy(&spawnParticleCount, d_spawnParticleCount, sizeof(int), cudaMemcpyDeviceToHost);
        cudaFree(d_spawnParticleCount);

        // Copy spawn particles back to host and append
        ParticlesHost spawnParticlesHost;
        spawnParticles.copyToHost(spawnParticlesHost, spawnParticleCount);

        // float v = cudaSpawn.area.size.x * cudaSpawn.area.size.y * cudaSpawn.area.size.z;
        // float m = cudaSpawn.restDensity * v / spawnParticleCount;
        //
        // for (auto& p : spawnParticlesHost.properties)
        //     p.x = m;

        allParticles.append(spawnParticlesHost);

        spawnParticles.free();
    }

    particleCount = allParticles.position.size();

    if (particleCount == 0) {
        state = UNINITIALIZED;
        return false;
    }

    tableSize = glm::nextPowerOfTwo(particleCount * 2);

    // Allocate memory on gpu
    particles.allocate(particleCount);
    sortedParticles.allocate(particleCount);

    cudaMalloc(reinterpret_cast<void**>(&d_particleHashes), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_sortedParticleHashes), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleIndices), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_sortedParticleIndices), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_offsets), tableSize * sizeof(int));

    // Allocate memory for CUB sorting
    cub::DeviceRadixSort::SortPairs(
        nullptr,
        sortTempStorageBytes,
        d_particleHashes, d_sortedParticleHashes,
        d_particleIndices, d_sortedParticleIndices, particleCount
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("QueryRequiredSortingStorage");
#endif

    cudaMalloc(&d_sortTempStorage, sortTempStorageBytes);

    allParticles.copyToDevice(particles, particleCount);
    allParticles.copyToDevice(sortedParticles, particleCount);

    cudaDeviceSynchronize();

    state = PAUSED;
    updateParticles();

    return true;
}

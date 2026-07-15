//
// Created by ruut on 3/16/26.
//

#include "simulation/FluidSimulation.h"
#include <algorithm>
#include <cuda_runtime.h>
#include <iostream>
#include <random>
#include <nvtx3/nvToolsExt.h>
#include "cuda/FluidSimulation.h"
#include "imgui.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"
#include "simulation/scene/nodes/ForceFieldNode.h"
#include "scene/Scene.h"
#include "cuda/CudaUtils.h"
#include <cuda_gl_interop.h>

#include "cuda/SimulationMetrics.h"
#include "scene/nodes/ParticlesNode.h"

// --- ParticlesHost ------------------------------------

void ParticlesHost::resize(size_t count) {
    position.resize(count);
    startPosition.resize(count);
    velocity.resize(count);
    properties.resize(count);
    flags.resize(count);
    color.resize(count);
    pressureForce.resize(count);
    viscosityForce.resize(count);
    surfaceTensionForce.resize(count);
}

void ParticlesHost::append(ParticlesHost& particles) {
    position.insert(position.end(), particles.position.begin(), particles.position.end());
    startPosition.insert(startPosition.end(), particles.startPosition.begin(), particles.startPosition.end());
    velocity.insert(velocity.end(), particles.velocity.begin(), particles.velocity.end());
    properties.insert(properties.end(), particles.properties.begin(), particles.properties.end());
    flags.insert(flags.end(), particles.flags.begin(), particles.flags.end());
    color.insert(color.end(), particles.color.begin(), particles.color.end());
    pressureForce.insert(pressureForce.end(), particles.pressureForce.begin(), particles.pressureForce.end());
    viscosityForce.insert(viscosityForce.end(), particles.viscosityForce.begin(), particles.viscosityForce.end());
    surfaceTensionForce.insert(surfaceTensionForce.end(), particles.surfaceTensionForce.begin(), particles.surfaceTensionForce.end());
}

void ParticlesHost::copyToDevice(const ParticlesGPU& device, const size_t count) const {
    // Copy data from host to device
    cudaMemcpy(device.d_position,               position.data(),               count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_startPosition,          startPosition.data(),          count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_velocity,               velocity.data(),               count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_properties,             properties.data(),             count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_flags,                  flags.data(),                  count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_color,                  color.data(),                  count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_pressureForce,          pressureForce.data(),          count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_viscosityForce,         viscosityForce.data(),         count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_surfaceTensionForce,    surfaceTensionForce.data(),    count * sizeof(float4), cudaMemcpyHostToDevice);
}

// --- ParticlesGPU -------------------------------------

void ParticlesGPU::allocate(const size_t count) {
    cudaMalloc(reinterpret_cast<void**>(&d_position),               count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_startPosition),          count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_velocity),               count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_properties),             count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_flags),                  count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_color),                  count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_pressureForce),          count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_viscosityForce),         count * sizeof(float4));
    cudaMalloc(reinterpret_cast<void**>(&d_surfaceTensionForce),    count * sizeof(float4));
}

void ParticlesGPU::free() {
    cudaFree(d_position           ); d_position             = nullptr;
    cudaFree(d_startPosition      ); d_startPosition        = nullptr;
    cudaFree(d_velocity           ); d_velocity             = nullptr;
    cudaFree(d_properties         ); d_properties           = nullptr;
    cudaFree(d_flags              ); d_flags                = nullptr;
    cudaFree(d_color              ); d_color                = nullptr;
    cudaFree(d_pressureForce      ); d_pressureForce        = nullptr;
    cudaFree(d_viscosityForce     ); d_viscosityForce       = nullptr;
    cudaFree(d_surfaceTensionForce); d_surfaceTensionForce  = nullptr;
}

void ParticlesGPU::copyToHost(ParticlesHost& host, const size_t count) const {
    host.resize(count);

    // Copy data from device to host
    cudaMemcpy(host.position.data(),            d_position,             count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.startPosition.data(),       d_startPosition,        count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.velocity.data(),            d_velocity,             count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.properties.data(),          d_properties,           count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.flags.data(),               d_flags,                count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.color.data(),               d_color,                count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.pressureForce.data(),       d_pressureForce,        count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.viscosityForce.data(),      d_viscosityForce,       count * sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.surfaceTensionForce.data(), d_surfaceTensionForce,  count * sizeof(float4), cudaMemcpyDeviceToHost);
}

void ParticlesGPU::copyToHost(ParticleHost& host, int id) const {
    cudaMemcpy(&host.position,              d_position + id,            sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.startPosition,         d_startPosition + id,       sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.velocity,              d_velocity + id,            sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.properties,            d_properties + id,          sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.flags,                 d_flags + id,               sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.color,                 d_color + id,               sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.pressureForce,         d_pressureForce + id,       sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.viscosityForce,        d_viscosityForce + id,      sizeof(float4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.surfaceTensionForce,   d_surfaceTensionForce + id, sizeof(float4), cudaMemcpyDeviceToHost);
}

// -----------------------------------------------------

FluidSimulation::FluidSimulation(Scene* scene, bool headless) : scene(scene), headless(headless) {
    cudaSetDevice(0);

    // Setup particles node for rendering
    if (!headless) {
        auto particlesNode = std::make_unique<ParticlesNode>(this);
        this->particlesNode = particlesNode.get();
        scene->addSceneNode(std::move(particlesNode));
    }
}

FluidSimulation::~FluidSimulation() {
    particles.free();
    sortedParticles.free();

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
        particles.d_position,
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
        particles,
        d_sortedParticleIndices,
        sortedParticles
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("ReorderParticlesKernel");
#endif

    launchDensityPressureKernel(
        particleCount,
        sortedParticles.d_position,
        sortedParticles.d_properties,
        d_sortedParticleHashes,
        d_offsets,
        sdfMap.d_cells != nullptr,
        sdfMap,
        volumeMap,
        scene->getSimulationDomain()->getCudaAABB(),
        settings.h,
        settings.speedSound,
        settings.p_0
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("DensityPressureKernel");
#endif

    launchSphForcesKernel(
        particleCount,
        sortedParticles.d_position,
        sortedParticles.d_properties,
        sortedParticles.d_velocity,
        sortedParticles.d_pressureForce,
        sortedParticles.d_viscosityForce,
        sortedParticles.d_surfaceTensionForce,
        sortedParticles.d_flags,
        d_sortedParticleHashes,
        d_offsets,
        sdfMap.d_cells != nullptr,
        sdfMap,
        volumeMap,
        scene->getSimulationDomain()->getCudaAABB(),
        settings.h,
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

void FluidSimulation::update(const float deltaTime) {
    if (state == UNINITIALIZED  || state == PAUSED)
        return;

    updateForceFieldBuffer();

    // --- Delta Time ---
    float dt = settings.useFixedTimeStep ? settings.fixedTimeStep / 1000.0f : deltaTime;

    for (int i = 0 ; i < settings.simulationSteps ; i++) {
        updateParticles();

        // --- Frame Metrics ---
        metrics.averageDensity = calculateAverageDensity(particleCount, particles.d_properties);

        metrics.averageDensityError = calculateAverageDensityError(
            particleCount,
            particles.d_properties,
            settings.p_0
        );

        metrics.densityStandardDeviation = calculateDensityStandardDeviation(particleCount, particles.d_properties, metrics.averageDensity);

        // --- CFL ---
        float cs = 10.0f * settings.maxVelocity;
        settings.speedSound = cs;

        float cflTimeStep = settings.safetyFactor * settings.h / (cs * (1.0f + 0.6f * settings.alpha));

        float maxAcc = calculateMaxAcceleration(particleCount, particles.d_pressureForce, particles.d_viscosityForce, particles.d_surfaceTensionForce);

        if (settings.enableGravity)
            maxAcc = fminf(maxAcc, abs(settings.gravity));

        float accTimeStep = 0.25f * sqrtf(settings.h / maxAcc);

        cfl = fminf(cflTimeStep, accTimeStep);

        launchUpdateParticlesKernel(
            particles.d_pressureForce,
            particles.d_viscosityForce,
            particles.d_surfaceTensionForce,
            particles.d_velocity,
            particles.d_position,
            particleCount,
            d_forceFields,
            scene->getForceFields().size(),
            settings.useCFL ? fminf(cfl, dt) : dt,
            scene->getSimulationDomain()->getCudaAABB(),
            settings.enableGravity ? settings.gravity : 0.0f
        );
    }

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
            particlesNode->setVisible(false);
            return;
        }

        particlesNode->setVisible(true);
        particlesNode->updateParticlesSize(particleCount);

        // Update particle rendering
        setRendererVBO(particlesNode->getParticleVBO());
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

    launchResetParticlesKernel(
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
    launchRenderPrepareKernel(
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

float FluidSimulation::getCFL() const {
    return cfl;
}

int FluidSimulation::getParticleCount() const {
    return particleCount;
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
        scene->getSimulationDomain()->getCudaAABB(),
        particleCount,
        particles.d_position,
        particles.d_properties,
        d_sortedParticleHashes,
        d_offsets,
        settings.h
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("VoxelFieldKernel");
#endif

    return true;
}

ParticleHost FluidSimulation::getParticle(const int id) const {
    if (state == UNINITIALIZED)
        return {};

    if (id < 0 || id >= particleCount)
        return {};

    ParticleHost p{};
    particles.copyToHost(p, id);
    return p;
}

FrameMetrics FluidSimulation::getFrameMetrics() const {
    return metrics;
}

bool FluidSimulation::initParticles() {
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
        CudaParticleSpawn cudaSpawn = spawn->getParticleSpawn();

        int countX          = std::floor((cudaSpawn.area.max.x - cudaSpawn.area.min.x) / cudaSpawn.particleSpacing + 1e-5f) + 1;
        int countY          = std::floor((cudaSpawn.area.max.y - cudaSpawn.area.min.y) / cudaSpawn.particleSpacing + 1e-5f) + 1;
        int countZ          = std::floor((cudaSpawn.area.max.z - cudaSpawn.area.min.z) / cudaSpawn.particleSpacing + 1e-5f) + 1;

        int maxSpawnParticles = countX * countY * countZ;

        ParticlesGPU spawnParticles;
        spawnParticles.allocate(maxSpawnParticles);

        int* d_spawnParticleCount;
        cudaMalloc(&d_spawnParticleCount, sizeof(int));
        cudaMemset(d_spawnParticleCount, 0, sizeof(int));

        launchInitParticlesKernel(
            cudaSpawn,
            spawnParticles,
            d_spawnParticleCount,
            scene->getSimulationDomain()->getCudaAABB(),
            { countX, countY, countZ }
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
        allParticles.append(spawnParticlesHost);

        spawnParticles.free();
    }

    particleCount = allParticles.position.size();

    if (particleCount == 0) {
        state = UNINITIALIZED;
        return false;
    }

    // Allocate memory on gpu
    particles.allocate(particleCount);
    sortedParticles.allocate(particleCount);

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

    allParticles.copyToDevice(particles, particleCount);
    allParticles.copyToDevice(sortedParticles, particleCount);

    cudaDeviceSynchronize();

    state = PAUSED;
    updateParticles();

    return true;
}

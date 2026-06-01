//
// Created by ruut on 3/16/26.
//

#include "FluidSimulation.h"
#include <algorithm>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <iostream>
#include <random>
#include <nvtx3/nvToolsExt.h>
#include "cuda/Boundaries.h"
#include "cuda/FluidSimulation.h"
#include "imgui.h"
#include "glad/gl.h"
#include "rendering/Scene.h"

FluidSimulation::FluidSimulation(Scene& scene) : scene(&scene) {
    cudaSetDevice(0);

    // Setup domain bounding box
    auto domainBB = std::make_unique<BoundingBoxNode>("Simulation Domain", glm::vec3(0.2f, 0.6f, 1.0f));
    domain = domainBB.get();
    domain->setScale({ 2.0f, 2.0f, 2.0f });
    domain->setDeletable(false);
    scene.addSceneNode(std::move(domainBB));
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

    cudaGraphicsUnmapResources(1, &rendererResource, nullptr);
}

void FluidSimulation::updateParticles() {
    if (state == UNINITIALIZED)
        return;

    // Reset old data
    cudaMemset(d_offsets,   -1, particleCount * sizeof(int));

    launchHashParticles(
        particleCount,
        d_particles,
        d_particleHashes,
        d_particleIndices,
        settings.h
    );

    sortParticlesByHash(
        particleCount,
        d_sortTempStorage,
        sortTempStorageBytes,
        d_particleHashes,
        d_sortedParticleHashes,
        d_particleIndices,
        d_sortedParticleIndices
    );

    launchBuildOffsetsKernel(
        particleCount,
        d_sortedParticleHashes,
        d_offsets
    );

    launchReorderParticlesKernel(
        particleCount,
        d_particles,
        d_sortedParticleIndices,
        d_sortedParticles
    );

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

    // Prepare particles for rendering
    size_t numBytes;
    cudaGraphicsMapResources(1, &rendererResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_rendererParticles), &numBytes, rendererResource);

    launchRenderPrepareKernel(
        particleCount,
        d_particles,
        d_rendererParticles
    );

    cudaGraphicsUnmapResources(1, &rendererResource);
}

void FluidSimulation::step(float deltaTime) {
    if (state == UNINITIALIZED)
        return;

    state = RUNNING;
    update(deltaTime);
    state = PAUSED;
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

    // Copy reset data to renderer vbo
    launchRenderPrepareKernel(
        particleCount,
        d_particles,
        d_rendererParticles
    );
}

void FluidSimulation::setRendererVBO(const GLuint vbo) {
    if (rendererResource) cudaGraphicsUnregisterResource(rendererResource);

    cudaGraphicsGLRegisterBuffer(
        &rendererResource,
        vbo,
        cudaGraphicsRegisterFlagsWriteDiscard
    );

    size_t numBytes;
    cudaGraphicsMapResources(1, &rendererResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_rendererParticles), &numBytes, rendererResource);

    launchRenderPrepareKernel(
        particleCount,
        d_particles,
        d_rendererParticles
    );

    cudaGraphicsUnmapResources(1, &rendererResource);
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

// void FluidSimulation::setBoundaryMaps(const Hex32Field& field) {
//     boundaryField = field;
// }

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

    launchVoxelFieldKernel(
        field,
        domain->getCudaAABB(),
        particleCount,
        d_particles,
        d_sortedParticleHashes,
        d_offsets,
        settings.h
    );

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
    // Delete old particle data
    particleCount = 0;

    if (d_particleHashes)           cudaFree(d_particleHashes);
    if (d_sortedParticleHashes)     cudaFree(d_sortedParticleHashes);
    if (d_sortedParticles)          cudaFree(d_sortedParticles);
    if (d_particleIndices)          cudaFree(d_particleIndices);
    if (d_sortedParticleIndices)    cudaFree(d_sortedParticleIndices);
    if (d_offsets)                  cudaFree(d_offsets);
    if (d_sortTempStorage)          cudaFree(d_sortTempStorage);

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

    cudaMalloc(&d_sortTempStorage, sortTempStorageBytes);

    cudaMemcpy(d_particles, allParticles.data(), particleCount * sizeof(ParticleGPU), cudaMemcpyHostToDevice);
    cudaMemcpy(d_sortedParticles, allParticles.data(), particleCount * sizeof(ParticleGPU), cudaMemcpyHostToDevice);

    cudaDeviceSynchronize();

    state = PAUSED;
    updateParticles();

    return true;
}
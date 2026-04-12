//
// Created by ruut on 3/16/26.
//

#include "FluidSimulationGPU.h"
#include <algorithm>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <iostream>
#include <ostream>
#include <random>
#include <nvtx3/nvToolsExt.h>
#include "cuda/FluidSimulation.h"
#include "glad/gl.h"

FluidSimulationGPU::FluidSimulationGPU() {
    cudaSetDevice(0);

    // Setup particles
    // particles.resize(initSettings.particleCount);
    // initParticlesRandom(x, y, z);
    // initParticles();

    // Determine h
    // float avgSpacing = std::sqrt(((x.y - x.x) * (y.y - y.x) * (z.y - z.x)) / particleCount);
    // settings.h = 5.0f * avgSpacing;

    // Setup ghost particles
    // int ghosts = 0;
    // auto addGhostLayer = [&](float3 start, float3 end, float spacing) {
    //     for (float x = start.x; x <= end.x; x += spacing) {
    //         for (float y = start.y; y <= end.y; y += spacing) {
    //             for (float z = start.z; z <= end.z; z += spacing) {
    //                 particles.push_back({
    //                     { x, y, z, 0.0f },
    //                     { x, y, z, 0.0f },
    //                     { 0.0f, 0.0f, 0.0f, 0.0f },
    //                     { 0.0256, 0.0f, 0.0f, 0.0f },
    //                     { GHOST, 0.0f, 0.0f, 0.0f },
    //                 });
    //                 ghosts++;
    //             }
    //         }
    //     }
    // };
    //
    // float s = 0.025f;                          // uniform spacing
    // float t = s * 3;                          // wall thickness (2 layers)
    // float xMin = settings.xBoundary.x;
    // float xMax = settings.xBoundary.y;
    // float yMin = settings.yBoundary.x;
    // float yMax = settings.yBoundary.y;
    // float zMin = settings.zBoundary.x;
    // float zMax = settings.zBoundary.y;
    //
    // // Floor (full coverage)
    // addGhostLayer({xMin - t, yMin - t, zMin - t}, {xMax + t, yMin, zMax + t}, s);
    //
    // // Left wall (exclude floor/ceiling overlap in Y)
    // addGhostLayer({xMin - t, yMin + s, zMin - t}, {xMin, yMax - s, zMax + t}, s);
    // // Right wall
    // addGhostLayer({xMax, yMin + s, zMin - t}, {xMax + t, yMax - s, zMax + t}, s);
    //
    // // Front wall (exclude corners already covered by X walls)
    // addGhostLayer({xMin, yMin + s, zMin - t}, {xMax, yMax - s, zMin}, s);
    // // Back wall
    // addGhostLayer({xMin, yMin + s, zMax}, {xMax, yMax - s, zMax + t}, s);

    // particlesCount = initSettings.particleCount + ghosts;

    // Allocate memory on gpu
    // cudaMalloc(reinterpret_cast<void**>(&d_particleHashes), particlesCount * sizeof(int));
    // cudaMalloc(reinterpret_cast<void**>(&d_sortedParticleHashes), particlesCount * sizeof(int));
    // cudaMalloc(reinterpret_cast<void**>(&d_particleIndices), particlesCount * sizeof(int));
    // cudaMalloc(reinterpret_cast<void**>(&d_sortedParticleIndices), particlesCount * sizeof(int));
    // cudaMalloc(reinterpret_cast<void**>(&d_offsets), particlesCount * sizeof(int));
    //
    // // Allocate memory for CUB sorting
    // queryRequiredSortingStorage(
        // particlesCount,
        // sortTempStorageBytes,
        // d_particleHashes,
        // d_sortedParticleHashes,
        // d_particleIndices,
        // d_sortedParticleIndices
    // );

    // cudaMalloc(&d_sortTempStorage, sortTempStorageBytes);
}

FluidSimulationGPU::~FluidSimulationGPU() {
    cudaFree(d_particles);
    cudaFree(d_sortedParticles);
    cudaFree(d_particleHashes);
    cudaFree(d_sortedParticleHashes);
    cudaFree(d_offsets);
    cudaFree(d_particleIndices);
    cudaFree(d_sortedParticleIndices);
    cudaFree(d_sortTempStorage);

    cudaGraphicsUnmapResources(1, &rendererResource, nullptr);
}

void FluidSimulationGPU::update(const float deltaTime) {
    if (state == UNINITIALIZED || state == PAUSED)
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
        settings.h,
        settings.k,
        settings.p_0
    );

    launchSphForcesKernel(
        particleCount,
        d_sortedParticles,
        d_sortedParticleHashes,
        d_offsets,
        settings.h,
        settings.mu,
        settings.tensionThreshold,
        settings.tension
    );

    // --- CFL ---
    float maxVelocity = getMaxVelocity(particleCount, d_sortedParticles);
    cfl = settings.safetyFactor * settings.h / (maxVelocity + 1e-5f);

    launchUpdateParticlesKernel(
        d_sortedParticles,
        particleCount,
        settings.useCFL ? fminf(cfl, deltaTime) : deltaTime,
        { 0.0, 1.0 },
        { 0.0, 1.0 },
        { 0.0, 1.0 },
        settings.enableGravity ? settings.gravity : 0.0f
    );

    size_t numBytes;
    cudaGraphicsMapResources(1, &rendererResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_rendererParticles), &numBytes, rendererResource);

    launchRenderPrepareKernel(
        particleCount,
        d_sortedParticles,
        d_rendererParticles
    );

    cudaGraphicsUnmapResources(1, &rendererResource);

    std::swap(d_particles, d_sortedParticles);
}

void FluidSimulationGPU::step(float deltaTime) {
    if (state == UNINITIALIZED)
        return;

    state = RUNNING;
    update(deltaTime);
    state = PAUSED;
}

void FluidSimulationGPU::reset() const {
    launchResetParticlesKernel(d_particles, particleCount);

    // Copy reset data to renderer vbo
    launchRenderPrepareKernel(
        particleCount,
        d_particles,
        d_rendererParticles
    );
}

void FluidSimulationGPU::setRendererVBO(const GLuint vbo) {
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

ParticleGPU FluidSimulationGPU::getParticle(const int id) const {
    ParticleGPU particle;

    if (state == UNINITIALIZED)
        return particle;

    if (id < 0 || id >= particleCount)
        return particle;

    cudaMemcpy(&particle, &d_particles[id], sizeof(ParticleGPU), cudaMemcpyDeviceToHost);
    return particle;
}

void FluidSimulationGPU::initParticles() {
    // Delete old particle data
    particleCount = 0;

    if (d_particleHashes)           cudaFree(d_particleHashes);
    if (d_sortedParticleHashes)     cudaFree(d_sortedParticleHashes);
    if (d_sortedParticles)          cudaFree(d_sortedParticles);
    if (d_particleIndices)          cudaFree(d_particleIndices);
    if (d_sortedParticleIndices)    cudaFree(d_sortedParticleIndices);
    if (d_offsets)                  cudaFree(d_offsets);
    if (d_sortTempStorage)          cudaFree(d_sortTempStorage);

    // Setup new particles
    float width         = initSettings.xBoundary.y - initSettings.xBoundary.x;
    float height        = initSettings.yBoundary.y - initSettings.yBoundary.x;
    float depth         = initSettings.zBoundary.y - initSettings.zBoundary.x;

    int countX          = std::floor(width  / initSettings.particleSpacing + 1e-5f) + 1;
    int countY          = std::floor(height / initSettings.particleSpacing + 1e-5f) + 1;
    int countZ          = std::floor(depth  / initSettings.particleSpacing + 1e-5f) + 1;

    particleCount       = countX * countY * countZ;

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

    float volume        = width * height * depth;
    float mass          = initSettings.totalMass * (volume / particleCount);

    // settings.h = 2.0f * initSettings.particleSpacing;

    // Set particle data
    launchInitParticlesKernel(
        particleCount,
        d_particles,
        { countX, countY, countZ },
        { initSettings.xBoundary.x, initSettings.yBoundary.x, initSettings.zBoundary.x },
        initSettings.particleSpacing,
        mass
    );

    cudaDeviceSynchronize();
    cudaMemcpy(d_sortedParticles, d_particles, particleCount * sizeof(ParticleGPU), cudaMemcpyDeviceToDevice);
    cudaDeviceSynchronize();

    state = PAUSED;
}

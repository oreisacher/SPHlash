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

FluidSimulationGPU::FluidSimulationGPU(int particleCount, glm::vec2 x, glm::vec2 y, glm::vec2 z)
    : particleCount(particleCount)
{
    cudaSetDevice(0);

    // Setup particles
    particles.resize(particleCount);
    initParticlesRandom(x, y, z);
    // initParticlesGrid(x, y, z);

    // Determine h
    float avgSpacing = std::sqrt(((x.y - x.x) * (y.y - y.x) * (z.y - z.x)) / particleCount);
    settings.h = 5.0f * avgSpacing;

    // Allocate memory on gpu
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
}

FluidSimulationGPU::~FluidSimulationGPU() {
    cudaFree(d_sortedParticles);
    cudaFree(d_particleHashes);
    cudaFree(d_sortedParticleHashes);
    cudaFree(d_offsets);
    cudaFree(d_particleIndices);
    cudaFree(d_sortedParticleIndices);
    cudaFree(d_sortTempStorage);

    cudaGraphicsUnmapResources(1, &particleResource, nullptr);
}

void FluidSimulationGPU::update(const float deltaTime) {
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
    cfl = 0.4f * settings.h / (sqrt(settings.k) + maxVelocity + 1e-6f);

    launchUpdateParticlesKernel(
        d_sortedParticles,
        particleCount,
        cfl,
        { settings.xBoundary.x, settings.xBoundary.y },
        { settings.yBoundary.x, settings.yBoundary.y },
        { settings.zBoundary.x, settings.zBoundary.y },
        settings.enableGravity ? settings.gravity : 0.0f
    );

    std::swap(d_particles, d_sortedParticles);
}

void FluidSimulationGPU::reset() const {
    launchResetParticlesKernel(d_particles, particleCount);
}

void FluidSimulationGPU::setupVBOConnection(const GLuint vbo) {
    cudaGraphicsGLRegisterBuffer(
        &particleResource,
        vbo,
        cudaGraphicsRegisterFlagsWriteDiscard
    );

    size_t numBytes = 0;
    cudaGraphicsMapResources(1, &particleResource);
    cudaGraphicsResourceGetMappedPointer(nullptr, &numBytes, particleResource);
    cudaGraphicsUnmapResources(1, &particleResource, nullptr);

    cudaMalloc(reinterpret_cast<void**>(&d_sortedParticles), numBytes);

    cudaGraphicsMapResources(1, &particleResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_particles), &numBytes, particleResource);
}

// --- Particle Inits ---
void FluidSimulationGPU::initParticlesRandom(glm::vec2 x, glm::vec2 y, glm::vec2 z) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(x.x, x.y);
    std::uniform_real_distribution<float> distY(y.x, y.y);
    std::uniform_real_distribution<float> distZ(z.x, z.y);

    float mass = 1000.0f * (((x.y - x.x) * (y.y - y.x) * (z.y - z.x)) / particleCount);

    for (int i = 0 ; i < particleCount ; i++) {
        particles[i].position = { distX(gen), distY(gen), distZ(gen), 0.0 };
        particles[i].startPosition = particles[i].position,
        particles[i].velocity = { 0.0, 0.0, 0.0, 0.0 };
        particles[i].properties = { mass, 0.0, 0.0, static_cast<float>(i) };
        particles[i].flags = { NORMAL, 0.0, 0.0, 0.0 };
    }
}

void FluidSimulationGPU::initParticlesGrid(glm::vec2 x, glm::vec2 y, glm::vec2 z) {
    int gridSize = static_cast<int>(std::sqrt(particleCount));
    float cellWidth  = (x.y - x.x) / gridSize;
    float cellHeight = (y.y - y.x) / gridSize;

    float mass = 1000.0f * (((x.y - x.x) * (y.y - y.x)) / particleCount);

    int index = 0;
    for (int gx = 0; gx < gridSize; gx++) {
        for (int gy = 0; gy < gridSize; gy++) {
            if (index >= particleCount) break;

            float posX = x.x + gx * cellWidth  + cellWidth  * 0.5f;
            float posY = y.x + gy * cellHeight + cellHeight * 0.5f;

            particles[index].position = { posX, posY, 0.0, 0.0 };
            particles[index].startPosition = particles[index].position;
            particles[index].velocity = { 0.0, 0.0, 0.0, 0.0 };
            particles[index].properties = { mass, 0.0, 0.0, 0.0 };
            particles[index].flags = { NORMAL, 0.0, 0.0, 0.0 };

            index++;
        }
    }
}

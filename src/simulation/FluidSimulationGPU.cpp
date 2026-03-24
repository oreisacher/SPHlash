//
// Created by ruut on 3/16/26.
//

#include "FluidSimulationGPU.h"

#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <iostream>
#include <ostream>
#include <random>
#include <nvtx3/nvToolsExt.h>

#include "cuda/FluidSimulation.h"
#include "glad/gl.h"

FluidSimulationGPU::FluidSimulationGPU(int particleCount, float xMin, float yMin, float xMax, float yMax) {
    // Setup particles
    this->particleCount = particleCount;
    particles.resize(particleCount);

    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_real_distribution<float> distX(xMin, xMax);
    // std::uniform_real_distribution<float> distY(yMin, yMax);
    //
    //
    // for (int i = 0 ; i < particleCount ; i++) {
    //     particles[i].position = { distX(gen), distY(gen), 0.0, 0.0 };
    //     particles[i].startPosition = particles[i].position,
    //     particles[i].velocity = { 0.0, 0.0, 0.0, 0.0 };
    //     particles[i].properties = { mass, 0.0, 0.0, 0.0 };
    // }

    int gridSize = static_cast<int>(std::sqrt(particleCount));
    float cellWidth  = (xMax - xMin) / gridSize;
    float cellHeight = (yMax - yMin) / gridSize;

    float mass = 1000.0f * (((xMax - xMin) * (yMax - yMin)) / particleCount);

    int index = 0;
    for (int gx = 0; gx < gridSize; gx++) {
        for (int gy = 0; gy < gridSize; gy++) {
            if (index >= particleCount) break;

            float x = xMin + gx * cellWidth  + cellWidth  * 0.5f;
            float y = yMin + gy * cellHeight + cellHeight * 0.5f;

            particles[index].position = { x, y, 0.0, 0.0 };
            particles[index].startPosition = particles[index].position;
            particles[index].velocity = { 0.0, 0.0, 0.0, 0.0 };
            particles[index].properties = { mass, 0.0, 0.0, 0.0 };

            index++;
        }
    }

    // Determine h
    float avgSpacing = std::sqrt(((xMax - xMin) * (yMax - yMin)) / (particleCount));
    settings.h = 2.0f * avgSpacing;

    // Setup hash data
    hashes.resize(particleCount);
    hashIndices.resize(particleCount);
    cellStart.resize(particleCount);
    cellEnd.resize(particleCount);

    cudaMalloc(reinterpret_cast<void**>(&d_hashes), hashes.size() * sizeof(uint32_t));
    cudaMalloc(reinterpret_cast<void**>(&d_hashIndices), hashIndices.size() * sizeof(uint32_t));
    cudaMalloc(reinterpret_cast<void**>(&d_cellStart), cellStart.size() * sizeof(int32_t));
    cudaMalloc(reinterpret_cast<void**>(&d_cellEnd), cellEnd.size() * sizeof(int32_t));

    cudaMemset(d_cellStart, -1, particleCount * sizeof(int32_t));
    cudaMemset(d_cellEnd,   -1, particleCount * sizeof(int32_t));
}

FluidSimulationGPU::~FluidSimulationGPU() {
    cudaFree(d_hashes);
    cudaFree(d_hashIndices);
    cudaFree(d_cellStart);
    cudaFree(d_cellEnd);
}

void FluidSimulationGPU::update(const float deltaTime) {
    ParticleGPU* d_particles = nullptr;
    size_t numBytes = 0;

    cudaGraphicsMapResources(1, &particleResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_particles), &numBytes, particleResource);

    // Reset old data
    cudaMemset(d_cellStart, -1, particleCount * sizeof(int32_t));
    cudaMemset(d_cellEnd,   -1, particleCount * sizeof(int32_t));

    // Hash particle position
    nvtxRangePush("Hashing");
    launchHashParticlesKernel(d_particles, d_hashes, d_hashIndices, settings.h, particleCount);
    nvtxRangePop();

    // Sort by hash to group particles -> Radix sort
    nvtxRangePush("Sorting");
    sortIndicesByHash(d_hashes, d_hashIndices, particleCount);
    nvtxRangePop();

    // Build lookup table
    nvtxRangePush("Lookup Table");
    launchFindHashCellBoundariesKernel(d_hashes, d_cellStart, d_cellEnd, particleCount);
    nvtxRangePop();

    // Actual Simulation
    nvtxRangePush("Density");
    launchDensityPressureKernel(d_particles, d_hashIndices, d_cellStart, d_cellEnd, settings.h, particleCount, settings.k, settings.p_0);
    nvtxRangePop();

    // SPH forces
    nvtxRangePush("SPH");
    launchSphForcesKernel(d_particles, d_hashIndices, d_cellStart, d_cellEnd, settings.h, particleCount, settings.mu, settings.tensionThreshold, settings.tension);
    nvtxRangePop();

    // Update particle positions
    nvtxRangePush("Update");
    launchUpdateParticlesKernel(d_particles, particleCount, deltaTime, width, height, settings.gravity);
    nvtxRangePop();

    cudaGraphicsUnmapResources(1, &particleResource, nullptr);
}

void FluidSimulationGPU::reset() {
    ParticleGPU* d_particles = nullptr;
    size_t numBytes = 0;

    cudaGraphicsMapResources(1, &particleResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_particles), &numBytes, particleResource);

    launchResetParticlesKernel(d_particles, particleCount);

    cudaGraphicsUnmapResources(1, &particleResource, nullptr);
}

void FluidSimulationGPU::updateBoundaries(float width, float height) {
    this->width = width;
    this->height = height;
}

void FluidSimulationGPU::setupVBOConnection(const GLuint vbo) {
    cudaGraphicsGLRegisterBuffer(
        &particleResource,
        vbo,
        cudaGraphicsMapFlagsWriteDiscard
    );
}
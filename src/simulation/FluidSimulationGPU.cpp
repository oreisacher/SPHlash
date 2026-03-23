//
// Created by ruut on 3/16/26.
//

#include "FluidSimulationGPU.h"

#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <iostream>
#include <ostream>
#include <random>

#include "cuda/FluidSimulation.h"
#include "glad/gl.h"

FluidSimulationGPU::FluidSimulationGPU(int particleCount, float xMin, float yMin, float xMax, float yMax) {
    // Setup particles
    this->particleCount = particleCount;
    particles.resize(particleCount);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(xMin, xMax);
    std::uniform_real_distribution<float> distY(yMin, yMax);

    float mass = 1000.0f * (((xMax - xMin) * (yMax - yMin)) / particleCount);

    for (int i = 0 ; i < particleCount ; i++) {
        particles[i].position = { distX(gen), distY(gen), 0.0, 0.0 };
        particles[i].startPosition = particles[i].position,
        particles[i].velocity = { 0.0, 0.0, 0.0, 0.0 };
        particles[i].properties = { mass, 0.0, 0.0, 0.0 };
    }

    // Determine h
    float avgSpacing = std::sqrt(((xMax - xMin) * (yMax - yMin)) / (particleCount));
    settings.h = 2.0f * avgSpacing;

    initParticlesSSBO();

    // Setup hash data
    hashes.resize(particleCount);
    hashIndices.resize(particleCount);
    cellStart.resize(particleCount);
    std::fill(cellStart.begin(), cellStart.end(), -1);
    cellEnd.resize(particleCount);
    std::fill(cellEnd.begin(), cellEnd.end(), -1);

    initHashDataSSBO();

    // Test dispatch
    // launchHashParticlesKernel(d_particles, d_hashes, d_hashIndices, settings.h, particleCount, particleCount);
    // cudaMemcpy(hashes.data(), d_hashes, hashes.size() * sizeof(uint32_t), cudaMemcpyDeviceToHost);
    // cudaMemcpy(hashIndices.data(), d_hashIndices, hashIndices.size() * sizeof(uint32_t), cudaMemcpyDeviceToHost);
    // sortIndicesByHash(d_hashes, d_hashIndices, particleCount);
    // launchFindHashCellBoundariesKernel(d_hashes, d_cellStart, d_cellEnd, particleCount);
    //
    // for (int i = 0 ; i < particleCount ; i++) {
    //     particles[i].properties.w = hashes[i];
    // }
}

FluidSimulationGPU::~FluidSimulationGPU() {
    // cudaFree(d_particles);
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
    launchHashParticlesKernel(d_particles, d_hashes, d_hashIndices, settings.h, particleCount);

    // Sort by hash to group particles -> Radix sort
    sortIndicesByHash(d_hashes, d_hashIndices, particleCount);

    // Build lookup table
    launchFindHashCellBoundariesKernel(d_hashes, d_cellStart, d_cellEnd, particleCount);

    // Actual Simulation
    launchDensityPressureKernel(d_particles, d_hashIndices, d_cellStart, d_cellEnd, settings.h, particleCount, settings.k, settings.p_0);

    // SPH forces
    launchSphForcesKernel(d_particles, d_hashIndices, d_cellStart, d_cellEnd, settings.h, particleCount, settings.mu, settings.tension);

    // Update particle positions
    launchUpdateParticlesKernel(d_particles, particleCount, deltaTime, width, height, settings.gravity);

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

void FluidSimulationGPU::setupVBOConnection(GLuint vbo) {
    cudaGraphicsGLRegisterBuffer(
        &particleResource,
        vbo,
        cudaGraphicsMapFlagsWriteDiscard
    );
}

void FluidSimulationGPU::initParticlesSSBO() {
    // cudaMalloc(reinterpret_cast<void**>(&d_particles), particles.size() * sizeof(ParticleGPU));
    // cudaMemcpy(d_particles, particles.data(), particles.size() * sizeof(ParticleGPU), cudaMemcpyHostToDevice);

    // glGenBuffers(1, &particleSSBO);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
    //
    // glBufferData(
    //     GL_SHADER_STORAGE_BUFFER,
    //     particles.size() * sizeof(ParticleGPU),
    //     particles.data(),
    //     GL_DYNAMIC_DRAW
    // );
    //
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
}

void FluidSimulationGPU::initHashDataSSBO() {
    cudaMalloc(reinterpret_cast<void**>(&d_hashes), hashes.size() * sizeof(uint32_t));
    cudaMalloc(reinterpret_cast<void**>(&d_hashIndices), hashIndices.size() * sizeof(uint32_t));
    cudaMalloc(reinterpret_cast<void**>(&d_cellStart), cellStart.size() * sizeof(int32_t));
    cudaMalloc(reinterpret_cast<void**>(&d_cellEnd), cellEnd.size() * sizeof(int32_t));

    cudaMemcpy(d_cellStart, cellStart.data(), cellStart.size() * sizeof(int32_t), cudaMemcpyHostToDevice);
    cudaMemcpy(d_cellEnd, cellEnd.data(), cellEnd.size() * sizeof(int32_t), cudaMemcpyHostToDevice);

    // glGenBuffers(1, &hashDataSSBO);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, hashDataSSBO);
    //
    // glBufferData(
    //     GL_SHADER_STORAGE_BUFFER,
    //     hashData.size() * sizeof(HashEntry),
    //     hashData.data(),
    //     GL_DYNAMIC_DRAW
    // );
    //
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, hashDataSSBO);
}

void FluidSimulationGPU::initHistogramDataSSBO() {
    // glGenBuffers(1, &histogramDataSSBO);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramDataSSBO);
    //
    // glBufferData(
    //     GL_SHADER_STORAGE_BUFFER,
    //     histogramData.size() * sizeof(uint32_t),
    //     histogramData.data(),
    //     GL_DYNAMIC_DRAW
    // );
    //
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, histogramDataSSBO);
}

void FluidSimulationGPU::initOffsetDataSSBO() {
    // glGenBuffers(1, &offsetDataSSBO);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, offsetDataSSBO);
    //
    // glBufferData(
    //     GL_SHADER_STORAGE_BUFFER,
    //     offsetData.size() * sizeof(uint32_t),
    //     offsetData.data(),
    //     GL_DYNAMIC_DRAW
    // );
    //
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, offsetDataSSBO);
}

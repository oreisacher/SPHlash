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

    float mass = 1000.0f * (((xMax - xMin) * (yMax - yMin)) / particleCount);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(xMin, xMax);
    std::uniform_real_distribution<float> distY(yMin, yMax);

    for (int i = 0 ; i < particleCount ; i++) {
        particles[i].position = { distX(gen), distY(gen), 0.0, 0.0 };
        particles[i].startPosition = particles[i].position,
        particles[i].velocity = { 0.0, 0.0, 0.0, 0.0 };
        particles[i].properties = { mass, 0.0, 0.0, 0.0 };
    }


    // int gridSize = static_cast<int>(std::sqrt(particleCount));
    // float cellWidth  = (xMax - xMin) / gridSize;
    // float cellHeight = (yMax - yMin) / gridSize;
    //
    // int index = 0;
    // for (int gx = 0; gx < gridSize; gx++) {
    //     for (int gy = 0; gy < gridSize; gy++) {
    //         if (index >= particleCount) break;
    //
    //         float x = xMin + gx * cellWidth  + cellWidth  * 0.5f;
    //         float y = yMin + gy * cellHeight + cellHeight * 0.5f;
    //
    //         particles[index].position = { x, y, 0.0, 0.0 };
    //         std::cout << x << " " << y << std::endl;
    //         particles[index].startPosition = particles[index].position;
    //         particles[index].velocity = { 0.0, 0.0, 0.0, 0.0 };
    //         particles[index].properties = { mass, 0.0, 0.0, 0.0 };
    //
    //         index++;
    //     }
    // }

    // Determine h
    float avgSpacing = std::sqrt(((xMax - xMin) * (yMax - yMin)) / (particleCount));
    settings.h = 5.0f * avgSpacing;

    updateBoundaries(1.0, 1.0f);

    // Setup hash data
    cudaMalloc(reinterpret_cast<void**>(&d_particleCellIndex), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleIndex), particleCount * sizeof(int));
}

FluidSimulationGPU::~FluidSimulationGPU() {
    cudaFree(d_particleCellIndex);
    cudaFree(d_particleIndex);
    cudaFree(d_cellStart);
    cudaFree(d_cellEnd);
}

void FluidSimulationGPU::update(const float deltaTime) {
    cudaGetLastError();

    ParticleGPU* d_particles = nullptr;
    size_t numBytes = 0;

    cudaGraphicsMapResources(1, &particleResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_particles), &numBytes, particleResource);

    // Reset old data
    cudaMemset(d_cellStart, -1, totalCells * sizeof(int));
    cudaMemset(d_cellEnd,   -1, totalCells * sizeof(int));

    // --- Fixed Grid ---
    nvtxRangePush("CellIndices");
    launchComputeCellIndexKernel(
        particleCount,
        d_particles,
        d_particleCellIndex,
        d_particleIndex,
        settings.h,
        { gridSizeX, gridSizeY }
    );
    nvtxRangePop();

    nvtxRangePush("Sort Cells");
    sortIndicesByCellId(
        particleCount,
        d_particleCellIndex,
        d_particleIndex
    );
    nvtxRangePop();

    nvtxRangePush("Cell Start End");
    launchFindCellStartEndKernel(
        particleCount,
        d_particleCellIndex,
        d_cellStart,
        d_cellEnd
    );
    nvtxRangePop();

    nvtxRangePush("Density");
    launchDensityPressureFixedGridKernel(
        particleCount,
        d_particles,
        d_particleIndex,
        d_cellStart,
        d_cellEnd,
        { gridSizeX, gridSizeY },
        settings.h,
        settings.k,
        settings.p_0
    );
    nvtxRangePop();

    nvtxRangePush("SPH");
    launchSphForcesFixedGridKernel(
        particleCount,
        d_particles,
        d_particleIndex,
        d_cellStart,
        d_cellEnd,
        { gridSizeX, gridSizeY },
        settings.h,
        settings.mu,
        settings.tensionThreshold,
        settings.tension
    );
    nvtxRangePop();

    nvtxRangePush("Mouse Force");
    launchApplyMouseForceKernel(
        particleCount,
        d_particles,
        { mouseX, mouseY },
        mouseRadius,
        mouseForceDirection
    );
    nvtxRangePop();

    nvtxRangePush("Update");
    launchUpdateParticlesKernel(
        d_particles,
        particleCount,
        deltaTime,
        width,
        height,
        settings.enableGravity ? settings.gravity : 0.0f
    );
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

void FluidSimulationGPU::updateMousePos(float x, float y) {
    this->mouseX = x;
    this->mouseY = height - y;
}

void FluidSimulationGPU::updateMouseForceDirection(float direction) {
    this->mouseForceDirection = direction;
}

void FluidSimulationGPU::updateMouseRadius(float direction) {
    this->mouseRadius += 0.01f * direction;
}

void FluidSimulationGPU::updateBoundaries(float width, float height) {
    this->width = width;
    this->height = height;

    // Recompute grid resolution
    gridSizeX = std::ceil(width / settings.h);
    gridSizeY = std::ceil(height / settings.h);
    totalCells = gridSizeX * gridSizeY;

    cudaDeviceSynchronize();

    cudaFree(d_cellStart);
    cudaFree(d_cellEnd);

    cudaMalloc(reinterpret_cast<void**>(&d_cellStart), totalCells * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_cellEnd), totalCells * sizeof(int));
}

void FluidSimulationGPU::setupVBOConnection(const GLuint vbo) {
    cudaGraphicsGLRegisterBuffer(
        &particleResource,
        vbo,
        cudaGraphicsMapFlagsWriteDiscard
    );
}
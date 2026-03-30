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
    : particleCount(particleCount), hashTableSize(4.0 * particleCount)
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
    cudaMalloc(reinterpret_cast<void**>(&d_cellStart), hashTableSize * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_cellEnd), hashTableSize * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleCellIndex1), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleCellIndex2), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleHashValue1), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleHashValue2), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleIndex1), particleCount * sizeof(int));
    cudaMalloc(reinterpret_cast<void**>(&d_particleIndex2), particleCount * sizeof(int));

    // Allocate memory for CUB sorting
    queryCubRadixBytes(
        particleCount,
        sortTempStorageBytes,
        d_particleHashValue1,
        d_particleHashValue2,
        d_particleIndex1,
        d_particleIndex2
    );

    cudaMalloc(&d_sortTempStorage, sortTempStorageBytes);
}

FluidSimulationGPU::~FluidSimulationGPU() {
    cudaFree(d_particleCellIndex1);
    cudaFree(d_particleCellIndex2);
    cudaFree(d_particleIndex1);
    cudaFree(d_particleIndex2);
    cudaFree(d_cellStart);
    cudaFree(d_cellEnd);
}

void FluidSimulationGPU::update(const float deltaTime) {
    ParticleGPU* d_particles = nullptr;
    size_t numBytes = 0;

    cudaGraphicsMapResources(1, &particleResource);
    cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&d_particles), &numBytes, particleResource);

    // Reset old data
    cudaMemset(d_cellStart, -1, hashTableSize * sizeof(int));
    cudaMemset(d_cellEnd,   -1, hashTableSize * sizeof(int));

    nvtxRangePush("CellIndices");
    // launchComputeCellIndexKernel(
    //     particleCount,
    //     d_particles,
    //     d_particleCellIndex1,
    //     d_particleIndex1,
    //     settings.h,
    //     { gridSizeX, gridSizeY, gridSizeZ }
    // );
    launchComputeCellIndexHashKernel(
        particleCount,
        d_particles,
        d_particleHashValue1,
        d_particleIndex1,
        settings.h,
        hashTableSize
    );
    nvtxRangePop();

    nvtxRangePush("Sort Cells");
    sortIndicesByCellId(
        particleCount,
        d_sortTempStorage,
        sortTempStorageBytes,
        d_particleHashValue1,
        d_particleHashValue2,
        d_particleIndex1,
        d_particleIndex2
    );
    nvtxRangePop();

    nvtxRangePush("Cell Start End");
    launchFindCellStartEndKernel(
        particleCount,
        d_particleHashValue2,
        d_cellStart,
        d_cellEnd
    );
    nvtxRangePop();

    launchReorderParticlesKernel(
        particleCount,
        d_particleIndex2,
        d_particles,
        d_sorted_particles
    );

    nvtxRangePush("Density");
    // launchDensityPressureFixedGridKernel(
    //     particleCount,
    //     d_particles,
    //     d_particleIndex2,
    //     d_cellStart,
    //     d_cellEnd,
    //     { gridSizeX, gridSizeY, gridSizeZ },
    //     settings.h,
    //     settings.k,
    //     settings.p_0
    // );
    launchDensityPressureHashKernel(
        particleCount,
        d_sorted_particles,
        d_particleIndex2,
        d_cellStart,
        d_cellEnd,
        hashTableSize,
        settings.h,
        settings.k,
        settings.p_0
    );
    nvtxRangePop();

    nvtxRangePush("SPH");
    // launchSphForcesFixedGridKernel(
    //     particleCount,
    //     d_particles,
    //     d_particleIndex2,
    //     d_cellStart,
    //     d_cellEnd,
    //     { gridSizeX, gridSizeY, gridSizeZ },
    //     settings.h,
    //     settings.mu,
    //     settings.tensionThreshold,
    //     settings.tension
    // );
    launchSphForcesHashKernel(
        particleCount,
        d_sorted_particles,
        d_particleIndex2,
        d_cellStart,
        d_cellEnd,
        hashTableSize,
        settings.h,
        settings.mu,
        settings.tensionThreshold,
        settings.tension
    );
    nvtxRangePop();

    // nvtxRangePush("Mouse Force");
    // launchApplyMouseForceKernel(
    //     particleCount,
    //     d_particles,
    //     { mouseX, mouseY },
    //     mouseRadius,
    //     mouseForceDirection
    // );
    // nvtxRangePop();

    // --- CFL ---
    float maxVelocity = computeMaxVelocity(d_sorted_particles, particleCount);
    float cfl = 0.4f * settings.h / (sqrt(settings.k) + maxVelocity + 1e-6f);

    nvtxRangePush("Update");
    launchUpdateParticlesKernel(
        d_sorted_particles,
        particleCount,
        cfl,
        { settings.xBoundary.x, settings.xBoundary.y },
        { settings.yBoundary.x, settings.yBoundary.y },
        { settings.zBoundary.x, settings.zBoundary.y },
        settings.enableGravity ? settings.gravity : 0.0f
    );
    nvtxRangePop();

    cudaMemcpy(d_particles, d_sorted_particles, particleCount * sizeof(ParticleGPU), cudaMemcpyDeviceToDevice);

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

// void FluidSimulationGPU::updateBoundaries() {
//     if (settings.h == 0.0)
//         return;
//
//     // float width = settings.xBoundary.y - settings.xBoundary.x;
//     // float height = settings.yBoundary.y - settings.yBoundary.x;
//     // float depth = settings.zBoundary.y - settings.zBoundary.x;
//     //
//     // // Recompute grid resolution
//     // gridSizeX = std::ceil(width / settings.h);
//     // gridSizeY = std::ceil(height / settings.h);
//     // gridSizeZ = std::ceil(depth / settings.h);
//     // totalCells = gridSizeX * gridSizeY * gridSizeZ;
//
//     cudaDeviceSynchronize();
//
//     cudaFree(d_cellStart);
//     cudaFree(d_cellEnd);
//
//     // cudaMalloc(reinterpret_cast<void**>(&d_cellStart), totalCells * sizeof(int));
//     // cudaMalloc(reinterpret_cast<void**>(&d_cellEnd), totalCells * sizeof(int));
//
//     cudaMalloc(reinterpret_cast<void**>(&d_cellStart), hashTableSize * sizeof(int));
//     cudaMalloc(reinterpret_cast<void**>(&d_cellEnd), hashTableSize * sizeof(int));
// }

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

    cudaMalloc(reinterpret_cast<void**>(&d_sorted_particles), numBytes);
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
        particles[i].properties = { mass, 0.0, 0.0, 0.0 };
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

            index++;
        }
    }
}

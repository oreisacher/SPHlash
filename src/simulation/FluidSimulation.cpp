//
// Created by ruut on 3/16/26.
//

#include "FluidSimulation.h"
#include <algorithm>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <iostream>
#include <ostream>
#include <random>
#include <nvtx3/nvToolsExt.h>

#include "cuda/Boundaries.h"
#include "cuda/FluidSimulation.h"

#include "imgui.h"
#include "glad/gl.h"
#include "../cuda/MarchingCubesLookup.h"
#include "rendering/Scene.h"

FluidSimulation::FluidSimulation(Scene& scene) : scene(&scene){
    cudaSetDevice(0);
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
        boundaryField,
        settings.domainArea.toCudaAABB(),
        settings.h,
        settings.k,
        settings.p_0
    );

    launchSphForcesKernel(
        particleCount,
        d_sortedParticles,
        d_sortedParticleHashes,
        d_offsets,
        boundaryField,
        settings.domainArea.toCudaAABB(),
        settings.h,
        settings.alpha,
        settings.speedSound,
        settings.tensionThreshold,
        settings.tension
    );

    std::swap(d_particles, d_sortedParticles);
}

void FluidSimulation::update(const float deltaTime) {
    if (state == UNINITIALIZED  || state == PAUSED)
        return;

    updateParticles();

    // --- CFL ---
    float maxVelocity = getMaxVelocity(particleCount, d_particles);
    cfl = settings.safetyFactor * settings.h / (settings.speedSound + maxVelocity + 1e-5f);

    launchCalculateVelocityKernel(
        d_particles,
        particleCount,
        d_forceFields,
        scene->getForceFields().size(),
        settings.useCFL ? fminf(cfl, deltaTime) : deltaTime,
        settings.enableGravity ? settings.gravity : 0.0f
    );

    launchXsphKernel(
        particleCount,
        d_particles,
        d_sortedParticleHashes,
        d_offsets,
        settings.h
    );

    launchUpdateParticlesKernel(
        d_particles,
        particleCount,
        d_forceFields,
        scene->getForceFields().size(),
        settings.useCFL ? fminf(cfl, deltaTime) : deltaTime,
        settings.domainArea.toCudaAABB(),
        settings.enableGravity ? settings.gravity : 0.0f
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

void FluidSimulation::step(float deltaTime) {
    if (state == UNINITIALIZED)
        return;

    state = RUNNING;
    update(deltaTime);
    state = PAUSED;
}

void FluidSimulation::start() {
    // Preprocess boundary density map
    // calculateDensityMap();

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

void FluidSimulation::rebuildForceFieldBuffer() {
    if (d_forceFields) cudaFree(d_forceFields);

    if (scene->getForceFields().size() == 0)
        return;

    cudaMalloc(&d_forceFields, scene->getForceFields().size() * sizeof(ForceField));
}

void FluidSimulation::updateForceFieldBuffer() const {
    cudaMemcpy(d_forceFields, scene->getForceFields().data(), scene->getForceFields().size() * sizeof(ForceField), cudaMemcpyHostToDevice);
}

void FluidSimulation::setBoundaryField(const Hex32Field& field) {
    boundaryField = field;
}

bool FluidSimulation::computeVoxelField(const VoxelField& field) const {
    if (particleCount == 0 || state == UNINITIALIZED)
        return false;

    launchVoxelFieldKernel(
        field,
        settings.domainArea.toCudaAABB(),
        particleCount,
        d_particles,
        d_sortedParticleHashes,
        d_offsets,
        settings.h
    );

    return true;
}

// void FluidSimulation::updateVoxelFieldSize(int3 resolution) {
//     if (voxelField.d_field) cudaFree(voxelField.d_field);
//     if (voxelField.d_normal) cudaFree(voxelField.d_normal);
//
//     int sampleCount = resolution.x * resolution.y * resolution.z;
//
//     cudaMalloc(&voxelField.d_field, sampleCount * sizeof(float));
//     cudaMalloc(&voxelField.d_normal, sampleCount * sizeof(float3));
// }

// VoxelField FluidSimulation::computeVoxelField(const int3 resolution) const {
    // if (particleCount == 0 || state == PAUSED)
    //     return {};
    //
    // launchVoxelFieldKernel(
    //     voxelField.d_field,
    //     voxelField.d_normal,
    //     resolution,
    //     { settings.xDomain.x, settings.yDomain.x, settings.zDomain.x },
    //     { settings.xDomain.y, settings.yDomain.y, settings.zDomain.y },
    //     particleCount,
    //     d_particles,
    //     d_sortedParticleHashes,
    //     d_offsets,
    //     settings.h
    // );

//     return {
//         voxelField.d_field,
//         voxelField.d_normal
//     };
// }

ParticleGPU FluidSimulation::getParticle(const int id) const {
    ParticleGPU particle;

    if (state == UNINITIALIZED)
        return particle;

    if (id < 0 || id >= particleCount)
        return particle;

    cudaMemcpy(&particle, &d_particles[id], sizeof(ParticleGPU), cudaMemcpyDeviceToHost);
    return particle;
}

void FluidSimulation::calculateDensityMap() {
    // // Get triangle data
    // std::vector<Triangle> meshTriangles;
    //
    // for (auto model : FluidRenderer::getInstance()->getModels()) {
    //     auto modelTriangles = model->getTriangles();
    //     meshTriangles.insert(meshTriangles.end(), modelTriangles.begin(), modelTriangles.end());
    // }
    //
    // std::cout << meshTriangles.size() << std::endl;
    //
    // Triangle* d_triangles;
    // cudaMalloc(&d_triangles, meshTriangles.size() * sizeof(Triangle));
    // cudaMemcpy(d_triangles, meshTriangles.data(), meshTriangles.size() * sizeof(Triangle), cudaMemcpyHostToDevice);
    //
    // // SDF
    // int sdfResolution = 64;
    //
    // float* d_sdf;
    // cudaMalloc(&d_sdf, sdfResolution * sdfResolution * sdfResolution * sizeof(float));
    //
    // glm::vec3 gridMin = { 0.0, 0.0, 0.0f };
    // glm::vec3 gridMax = { 1.0, 1.0, 1.0f };
    //
    // launchSdfKernel(
    //     meshTriangles.size(),
    //     d_triangles,
    //     d_sdf,
    //     sdfResolution * sdfResolution * sdfResolution,
    //     sdfResolution,
    //     { gridMin.x, gridMin.y, gridMin.z },
    //     { gridMax.x, gridMax.y, gridMax.z }
    // );
    //
    // std::vector<float> sdf(sdfResolution * sdfResolution * sdfResolution);
    // cudaMemcpy(sdf.data(), d_sdf, sdf.size() * sizeof(float), cudaMemcpyDeviceToHost);
    //
    // cudaDeviceSynchronize();

    // Marching cubes
    // float3 cellSize = make_float3(
    //     (gridMax.x - gridMin.x) / (sdfResolution - 1),
    //     (gridMax.y - gridMin.y) / (sdfResolution - 1),
    //     (gridMax.z - gridMin.z) / (sdfResolution - 1)
    // );
    //
    // int dx = 1;
    // int dy = sdfResolution;
    // int dz = sdfResolution * sdfResolution;
    //
    // std::vector<Vertex> vertices = {};
    // std::vector<unsigned int> indices = {};
    //
    // for (int z = 0; z < sdfResolution - 1; z++) {
    //     for (int y = 0; y < sdfResolution - 1; y++) {
    //         for (int x = 0; x < sdfResolution - 1; x++) {
    //             int id = x + y * sdfResolution + z * sdfResolution * sdfResolution;
    //
    //             float cube[8];
    //             cube[0] = sdf[id                ];
    //             cube[1] = sdf[id + dx           ];
    //             cube[2] = sdf[id + dx + dy      ];
    //             cube[3] = sdf[id      + dy      ];
    //             cube[4] = sdf[id           + dz ];
    //             cube[5] = sdf[id + dx      + dz ];
    //             cube[6] = sdf[id + dx + dy + dz ];
    //             cube[7] = sdf[id      + dy + dz ];
    //
    //             // Build index
    //             int cubeIndex = 0;
    //             if (cube[0] < 0) cubeIndex |= 1;
    //             if (cube[1] < 0) cubeIndex |= 2;
    //             if (cube[2] < 0) cubeIndex |= 4;
    //             if (cube[3] < 0) cubeIndex |= 8;
    //             if (cube[4] < 0) cubeIndex |= 16;
    //             if (cube[5] < 0) cubeIndex |= 32;
    //             if (cube[6] < 0) cubeIndex |= 64;
    //             if (cube[7] < 0) cubeIndex |= 128;
    //
    //             // Skip empty / full cells
    //             if (cubeIndex == 0 || cubeIndex == 255)
    //                 continue;
    //
    //             glm::vec3 position[8];
    //             position[0] = glm::vec3((x + 0) * cellSize.x, (y + 0) * cellSize.y, (z + 0)* cellSize.z) + gridMin;
    //             position[1] = glm::vec3((x + 1) * cellSize.x, (y + 0) * cellSize.y, (z + 0)* cellSize.z) + gridMin;
    //             position[2] = glm::vec3((x + 1) * cellSize.x, (y + 1) * cellSize.y, (z + 0) * cellSize.z) + gridMin;
    //             position[3] = glm::vec3((x + 0) * cellSize.x, (y + 1) * cellSize.y, (z + 0) * cellSize.z) + gridMin;
    //             position[4] = glm::vec3((x + 0) * cellSize.x, (y + 0) * cellSize.y, (z + 1) * cellSize.z) + gridMin;
    //             position[5] = glm::vec3((x + 1) * cellSize.x, (y + 0) * cellSize.y, (z + 1) * cellSize.z) + gridMin;
    //             position[6] = glm::vec3((x + 1) * cellSize.x, (y + 1) * cellSize.y, (z + 1) * cellSize.z) + gridMin;
    //             position[7] = glm::vec3((x + 0) * cellSize.x, (y + 1) * cellSize.y, (z + 1) * cellSize.z) + gridMin;
    //
    //             // Add vertices
    //             int edgeVertexIndex[12] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    //             for (int i = 0 ; i < 12 ; i++) {
    //                 if ((edgeTable[cubeIndex] & (1 << i)) == 0)
    //                     continue;
    //
    //                 auto verts = edgeToVertices[i];
    //
    //                 auto lerpedVertex = interp(
    //                     position[verts.first],
    //                     position[verts.second],
    //                     cube[verts.first],
    //                     cube[verts.second]
    //                 );
    //
    //                 edgeVertexIndex[i] = vertices.size();
    //                 vertices.push_back({{ lerpedVertex }, { calculateNormal(x, y, z, sdfResolution, sdf) }});
    //             }
    //
    //             // Build faces
    //             for (int i = 0 ; triangleTable[cubeIndex][i] != -1 ; i+= 3) {
    //                 int index0 = triangleTable[cubeIndex][i    ];
    //                 int index1 = triangleTable[cubeIndex][i + 1];
    //                 int index2 = triangleTable[cubeIndex][i + 2];
    //
    //                 indices.push_back(edgeVertexIndex[index0]);
    //                 indices.push_back(edgeVertexIndex[index2]);
    //                 indices.push_back(edgeVertexIndex[index1]);
    //             }
    //         }
    //     }
    // }
    //
    // std::vector<Mesh*> meshes = { new Mesh("SDF", vertices, indices) };
    // auto sdfModel = new Model("SDF", meshes);
    // FluidRenderer::getInstance()->addModel(sdfModel);
}

void FluidSimulation::initParticles() {
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
    glm::vec3 spawnAreaSize = initSettings.spawnArea.size;

    int countX          = std::floor(spawnAreaSize.x / initSettings.particleSpacing + 1e-5f) + 1;
    int countY          = std::floor(spawnAreaSize.y / initSettings.particleSpacing + 1e-5f) + 1;
    int countZ          = std::floor(spawnAreaSize.z / initSettings.particleSpacing + 1e-5f) + 1;

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

    // TODO - Check if this is correct
    float mass = initSettings.totalMass * (initSettings.spawnArea.volume() / particleCount);
    // float mass = initSettings.totalMass / particleCount;
    glm::vec3 spawnAreaMinimum = initSettings.spawnArea.min();

    // Set particle data
    launchInitParticlesKernel(
        particleCount,
        d_particles,
        { countX, countY, countZ },
        { spawnAreaMinimum.x, spawnAreaMinimum.y, spawnAreaMinimum.z },
        initSettings.particleSpacing,
        mass
    );

    cudaDeviceSynchronize();
    cudaMemcpy(d_sortedParticles, d_particles, particleCount * sizeof(ParticleGPU), cudaMemcpyDeviceToDevice);
    cudaDeviceSynchronize();

    state = PAUSED;

    // Update density, pressure, etc. once to ensure proper marching cubes rendering
    updateParticles();
}

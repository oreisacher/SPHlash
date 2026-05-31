//
// Created by ruut on 4/27/26.
//

#include "FluidSurfaceMesher.h"
#include "cuda/MarchingCubes.h"
#include <cuda_gl_interop.h>
#include <iostream>
#include "Scene.h"

FluidSurfaceMesher::FluidSurfaceMesher(Scene& scene, FluidSimulation* simulation, FluidRenderer* renderer)
    : renderer(renderer), simulation(simulation)
{
    // Init voxel field
    updateVoxelFieldSize();

    cudaMalloc(&d_vertexCount, sizeof(int));
    cudaMalloc(&d_triangleCount, sizeof(int));

    // Marching cubes
    int numCubes =
        (settings.resolution.x - 1) *
        (settings.resolution.y - 1) *
        (settings.resolution.z - 1);

    int estimatedActiveCubes = numCubes * 0.15f;

    int maxVertices = estimatedActiveCubes * 15 * 1.2f;
    int maxIndices = estimatedActiveCubes * 15 * 1.2f;

    // Create Surface Model
    auto surfaceMesh = std::make_unique<Mesh>("Surface Mesh", maxVertices, maxIndices);
    this->surfaceMesh = surfaceMesh.get();

    auto surfaceModel = std::make_unique<Model>(
        "Surface",
        std::move(surfaceMesh)
    );
    this->surfaceModel = surfaceModel.get();

    Material surfaceMaterial("Surface", Shader::createDefaultShaded());
    surfaceMaterial.setVec3("albedo", glm::vec3(0.2f, 0.6f, 1.0f));
    surfaceModel->addMaterial(surfaceMaterial);

    // Default state invisible
    surfaceModel->setVisible(false);
    surfaceModel->setSceneGraphVisible(false);

    scene.addModel(std::move(surfaceModel));
}

FluidSurfaceMesher::~FluidSurfaceMesher() {
    cudaFree(d_vertexCount);
    cudaFree(d_triangleCount);
}

void FluidSurfaceMesher::generateMesh(bool force) const {
    if (!force && (!surfaceModel->isVisible() || simulation->state == PAUSED))
        return;

    if (!simulation->computeVoxelField(voxelField))
        return;

    cudaMemset(  d_vertexCount, 0, sizeof(int));
    cudaMemset(d_triangleCount, 0, sizeof(int));

    auto verticesResource = surfaceMesh->getVerticesResource();
    auto indicesResource = surfaceMesh->getIndicesResource();

    cudaGraphicsMapResources(1, &verticesResource, nullptr);
    cudaGraphicsMapResources(1, &indicesResource, nullptr);

    // --- Marching Cubes ---
    Vertex* d_vertices;
    unsigned int* d_indices;
    size_t vboSize, eboSize;

    cudaGraphicsResourceGetMappedPointer((void**)&d_vertices, &vboSize, verticesResource);
    cudaGraphicsResourceGetMappedPointer((void**) &d_indices, &eboSize,  indicesResource);

    launchMarchingCubesKernel(
        voxelField,
        simulation->getDomain()->getCudaAABB(),
        d_vertexCount,
        d_triangleCount,
        d_vertices,
        d_indices,
        surfaceMesh->getMaxVertexCount(),
        surfaceMesh->getMaxIndexCount(),
        settings.isoLevel,
        settings.smoothNormals
    );

    // Unmap resources
    cudaGraphicsUnmapResources(1, &verticesResource, nullptr);
    cudaGraphicsUnmapResources(1, &indicesResource, nullptr);

    int vertexCount, triangleCount;
    cudaMemcpy(  &vertexCount,   d_vertexCount, sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(&triangleCount, d_triangleCount, sizeof(int), cudaMemcpyDeviceToHost);

    // Update vertex & indices data within mesh to ensure proper rendering
    surfaceMesh->setVertexCount(vertexCount);
    surfaceMesh->setIndexCount(triangleCount * 3);
}

void FluidSurfaceMesher::updateVoxelFieldSize() {
    if (settings.resolution.x <= 0 || settings.resolution.y <= 0 || settings.resolution.z <= 0)
        return;

    if (voxelField.d_color) cudaFree(voxelField.d_color);
    if (voxelField.d_normal) cudaFree(voxelField.d_normal);

    // Populate voxelfield
    voxelField.resolution = {
        settings.resolution.x,
        settings.resolution.y,
        settings.resolution.z
    };

    int sampleCount =
        voxelField.resolution.x *
        voxelField.resolution.y *
        voxelField.resolution.z;

    // Allocate device memory for cells
    cudaMalloc(&voxelField.d_color, sampleCount * sizeof(float));
    cudaMalloc(&voxelField.d_normal, sampleCount * sizeof(float3));

    generateMesh(true);
}

void FluidSurfaceMesher::updateSettings() const {
    if (renderer->settings.mode != MARCHING_CUBES) {
        surfaceModel->setVisible(false);
    } else {
        surfaceModel->setVisible(true);
        surfaceModel->getMaterial(0).setVec4("albedo", settings.color);

        // Force generate new surface mesh after switching mode to avoid mesh based on old particle data
        generateMesh(true);
    }
}

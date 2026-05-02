//
// Created by ruut on 4/27/26.
//

#include "FluidSurfaceMesher.h"

#include "cuda/MarchingCubes.h"

FluidSurfaceMesher::FluidSurfaceMesher() {
    renderer = FluidRenderer::getInstance();
    simulation = FluidSimulation::getInstance();

    // Init voxel field
    simulation->updateScalarFieldSize(renderer->settings.marchingCubeResolution);

    // Marching cubes
    int numCubes =
        (renderer->settings.marchingCubeResolution.x - 1) *
        (renderer->settings.marchingCubeResolution.y - 1) *
        (renderer->settings.marchingCubeResolution.z - 1);

    int estimatedActiveCubes = numCubes * 0.15f;

    int maxVertices = estimatedActiveCubes * 15 * 1.2f;
    int maxIndices = estimatedActiveCubes * 15 * 1.2f;

    surfaceMesh = new Mesh("Surface", maxVertices, maxIndices);
    // renderer->setSurfaceModel(new Model("Surface", { surfaceMesh }));
}

void FluidSurfaceMesher::generateMesh() const {
    // if (renderer->settings.mode != SOLID_COLOR)
    //     return;
    //
    // VoxelField field = simulation->computeScalarField(renderer->settings.marchingCubeResolution);
    //
    // if (field.d_field == nullptr || field.d_normal == nullptr)
    //     return;
    //
    // int* d_vertexCount;
    // int* d_triangleCount;
    // cudaMalloc(&d_vertexCount, sizeof(int));
    // cudaMalloc(&d_triangleCount, sizeof(int));
    // cudaMemset(d_vertexCount, 0, sizeof(int));
    // cudaMemset(d_triangleCount, 0, sizeof(int));
    //
    // cudaGraphicsMapResources(1, &surfaceMesh->vboResource, nullptr);
    // cudaGraphicsMapResources(1, &surfaceMesh->eboResource, nullptr);
    //
    // Vertex* d_vertices;
    // unsigned int* d_indices;
    // size_t vboSize, eboSize;
    //
    // cudaGraphicsResourceGetMappedPointer((void**)&d_vertices, &vboSize, surfaceMesh->vboResource);
    // cudaGraphicsResourceGetMappedPointer((void**)&d_indices, &eboSize, surfaceMesh->eboResource);
    //
    // launchMarchingCubesKernel(
    //     simulation->voxelField.d_field,
    //     simulation->voxelField.d_normal,
    //     renderer->settings.marchingCubeResolution,
    //     { simulation->settings.xDomain.x, simulation->settings.yDomain.x, simulation->settings.zDomain.x },
    //     { simulation->settings.xDomain.y, simulation->settings.yDomain.y, simulation->settings.zDomain.y },
    //     d_vertexCount,
    //     d_triangleCount,
    //     d_vertices,
    //     d_indices,
    //     surfaceMesh->maxVertices,
    //     surfaceMesh->maxIndices
    // );
    //
    // // Unmap resources
    // cudaGraphicsUnmapResources(1, &surfaceMesh->vboResource, nullptr);
    // cudaGraphicsUnmapResources(1, &surfaceMesh->eboResource, nullptr);
    //
    // int vertexCount, triangleCount;
    // cudaMemcpy(&vertexCount, d_vertexCount, sizeof(int), cudaMemcpyDeviceToHost);
    // cudaMemcpy(&triangleCount, d_triangleCount, sizeof(int), cudaMemcpyDeviceToHost);
    //
    // surfaceMesh->vertexCount = vertexCount;
    // surfaceMesh->indexCount = triangleCount * 3;
    //
    // cudaFree(d_vertexCount);
    // cudaFree(d_triangleCount);
}

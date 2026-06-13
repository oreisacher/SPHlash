//
// Created by ruut on 5/9/26.
//

#include "simulation/boundaries/BoundaryBuilder.h"
#include <cuda_runtime.h>
#include <vector_functions.h>
#include "assets/Triangle.h"
#include "cuda/Boundaries.h"
#include "cuda/CudaUtils.h"
#include "glad/gl.h"
#include "simulation/scene/nodes/ModelNode.h"

bool BoundaryBuilder::constructSDFMap(const Scene* scene, CudaAABB domain, float h, Hex32Field& sdfMap) const {
    // Extract list of all triangles from boundary objects of scene
    std::vector<Triangle> triangles;

    for (const auto& model : scene->getModels()) {
        if (!model->isBoundary())
            continue;

        auto modelTriangles = model->getWorldSpaceTriangles();
        triangles.insert(triangles.end(), modelTriangles.begin(), modelTriangles.end());
    }

    if (triangles.empty())
        return false;

    Triangle* d_triangles;
    cudaMalloc(&d_triangles, triangles.size() * sizeof(Triangle));
    cudaMemcpy(d_triangles, triangles.data(), triangles.size() * sizeof(Triangle), cudaMemcpyHostToDevice);

    // Calculate SDF field of scene
    sdfMap.resolution = make_int3(settings.resolution.x, settings.resolution.y, settings.resolution.z);
    cudaMalloc(&(sdfMap.d_cells), sdfMap.resolution.x * sdfMap.resolution.y * sdfMap.resolution.z * sizeof(Hex32Cell));

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    launchSdfKernel(
        triangles.size(),
        d_triangles,
        sdfMap,
        domain,
        h
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK("SdfKernel");
#endif

    cudaFree(d_triangles);

    return true;
}

bool BoundaryBuilder::constructVolumeMap(Hex32Field sdfHexField, CudaAABB domain, float h, Hex32Field& volumeMap) const {
    volumeMap.resolution = sdfHexField.resolution;
    cudaMalloc(&(volumeMap.d_cells), sdfHexField.resolution.x * sdfHexField.resolution.y * sdfHexField.resolution.z * sizeof(Hex32Cell));

#ifdef DEBUG_CUDA
    cudaGetLastError();
#endif

    launchVolumeMapKernel(
        sdfHexField,
        volumeMap,
        domain,
        h
    );

#ifdef DEBUG_CUDA
    CUDA_KERNEL_CHECK_ACT("VolumeMapKernel", return false);
#endif

    // Copy data of volume map to 3d texture for debug viewing
    std::vector<Hex32Cell> cells(volumeMap.resolution.x * volumeMap.resolution.y * volumeMap.resolution.z);
    cudaMemcpy(cells.data(), volumeMap.d_cells, volumeMap.resolution.x * volumeMap.resolution.y * volumeMap.resolution.z * sizeof(Hex32Cell), cudaMemcpyDeviceToHost);

    std::vector<float> cellData;
    cellData.reserve(volumeMap.resolution.x * volumeMap.resolution.y * volumeMap.resolution.z);

    for (auto& cell : cells)
        cellData.push_back(cell.nodes[0] * 1000.0f);    // Normalize value between [0, 1] to ensure proper rendering

    glGenTextures(1, &volumeMap.texture);
    glBindTexture(GL_TEXTURE_3D, volumeMap.texture);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexImage3D(
       GL_TEXTURE_3D,
       0,
       GL_R32F,
       settings.resolution.x,
       settings.resolution.y,
       settings.resolution.z,
       0,
       GL_RED,
       GL_FLOAT,
       cellData.data()
    );

    return true;
}
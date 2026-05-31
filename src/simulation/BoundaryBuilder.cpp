//
// Created by ruut on 5/9/26.
//

#include "BoundaryBuilder.h"

#include <iostream>
#include <thread>

#include "imgui_internal.h"
#include "cuda/Boundaries.h"

Hex32Field BoundaryBuilder::constructSDFMap(const Scene* scene, CudaAABB domain, float h) const {
    // Extract list of all triangles from boundary objects of scene
    std::vector<Triangle> triangles;

    for (const auto& model : scene->getBoundaries()) {
        auto modelTriangles = model->getWorldSpaceTriangles();
        triangles.insert(triangles.end(), modelTriangles.begin(), modelTriangles.end());
    }

    if (triangles.empty())
        return {};

    Triangle* d_triangles;
    cudaMalloc(&d_triangles, triangles.size() * sizeof(Triangle));
    cudaMemcpy(d_triangles, triangles.data(), triangles.size() * sizeof(Triangle), cudaMemcpyHostToDevice);

    // Calculate SDF field of scene
    Hex32Field sdfHexField;
    sdfHexField.resolution = make_int3(settings.resolution.x, settings.resolution.y, settings.resolution.z);
    cudaMalloc(&(sdfHexField.d_cells), sdfHexField.resolution.x * sdfHexField.resolution.y * sdfHexField.resolution.z * sizeof(Hex32Cell));

    launchSdfKernel(
        triangles.size(),
        d_triangles,
        sdfHexField,
        domain,
        h
    );

    cudaFree(d_triangles);

    return sdfHexField;
}

Hex32Field BoundaryBuilder::constructVolumeMap(Hex32Field sdfMap, CudaAABB domain, float h) const {
    Hex32Field volumeHexField;
    volumeHexField.resolution = sdfMap.resolution;
    cudaMalloc(&(volumeHexField.d_cells), sdfMap.resolution.x * sdfMap.resolution.y * sdfMap.resolution.z * sizeof(Hex32Cell));

    launchVolumeMapKernel(
        sdfMap,
        volumeHexField,
        domain,
        h
    );

    // Copy data of volume map to 3d texture for debug viewing
    std::vector<Hex32Cell> cells(volumeHexField.resolution.x * volumeHexField.resolution.y * volumeHexField.resolution.z);
    cudaMemcpy(cells.data(), volumeHexField.d_cells, volumeHexField.resolution.x * volumeHexField.resolution.y * volumeHexField.resolution.z * sizeof(Hex32Cell), cudaMemcpyDeviceToHost);

    std::vector<float> cellData;
    cellData.reserve(volumeHexField.resolution.x * volumeHexField.resolution.y * volumeHexField.resolution.z);

    for (auto& cell : cells)
        cellData.push_back(cell.nodes[0] * 1000.0f);    // Normalize value between [0, 1] to ensure proper rendering

    glGenTextures(1, &volumeHexField.texture);
    glBindTexture(GL_TEXTURE_3D, volumeHexField.texture);

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

    return volumeHexField;
}
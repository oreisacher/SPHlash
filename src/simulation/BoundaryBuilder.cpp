//
// Created by ruut on 5/9/26.
//

#include "BoundaryBuilder.h"

#include <iostream>
#include <thread>

#include "imgui_internal.h"
#include "cuda/Boundaries.h"

Hex32Field BoundaryBuilder::constructDensityMap(const Scene* scene, AABB domain, float h, float restDensity) const {
    // Extract list of all triangles from boundary objects of scene
    std::vector<Triangle> triangles;

    for (const auto& model : scene->getModels()) {
        if (model->getType() != Type::BOUNDARY)
            continue;

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
        domain.toCudaAABB(),
        h
    );

    // Estimate density map of SDF field
    Hex32Field densityHexField;
    densityHexField.resolution = sdfHexField.resolution;
    cudaMalloc(&(densityHexField.d_cells), sdfHexField.resolution.x * sdfHexField.resolution.y * sdfHexField.resolution.z * sizeof(Hex32Cell));

    launchDensityMapKernel(
        sdfHexField,
        densityHexField,
        domain.toCudaAABB(),
        settings.samples,
        restDensity,
        h
    );

    // Copy data of density map to 3d texture for debug viewing
    std::vector<Hex32Cell> cells(densityHexField.resolution.x * densityHexField.resolution.y * densityHexField.resolution.z);
    cudaMemcpy(cells.data(), densityHexField.d_cells, densityHexField.resolution.x * densityHexField.resolution.y * densityHexField.resolution.z * sizeof(Hex32Cell), cudaMemcpyDeviceToHost);

    std::vector<float> cellData;
    cellData.reserve(densityHexField.resolution.x * densityHexField.resolution.y * densityHexField.resolution.z);

    for (auto& cell : cells)
        cellData.push_back(cell.nodes[0] / restDensity);    // Normalize value between [0, 1] to ensure proper rendering

    glGenTextures(1, &densityHexField.texture);
    glBindTexture(GL_TEXTURE_3D, densityHexField.texture);

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

    cudaFree(d_triangles);
    cudaFree(sdfHexField.d_cells);

    return densityHexField;
}
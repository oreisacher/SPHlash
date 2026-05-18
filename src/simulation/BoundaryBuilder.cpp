//
// Created by ruut on 5/9/26.
//

#include "BoundaryBuilder.h"

#include <iostream>

#include "imgui_internal.h"
#include "cuda/Boundaries.h"

BoundaryBuilder::BoundaryBuilder() {
    // Fill 3d texture
    glGenTextures(1, &tex3D);
    glBindTexture(GL_TEXTURE_3D, tex3D);
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
        nullptr
    );
}

Hex32Field BoundaryBuilder::constructDensityMap(const Scene& scene, AABB domain) {
    std::cout << "Building density map" << std::endl;

    // Extract list of all triangles from boundary objects of scene
    std::vector<Triangle> triangles;

    for (auto model : scene.getModels()) {
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

    // Build Hex32Field
    Hex32Field hexField;
    hexField.resolution = make_int3(
        settings.resolution.x,
        settings.resolution.y,
        settings.resolution.z
    );

    cudaMalloc(&(hexField.d_cells), hexField.resolution.x * hexField.resolution.y * hexField.resolution.z * sizeof(Hex32Cell));

    // Launch kernel
    launchSdfKernel(
        triangles.size(),
        d_triangles,
        hexField,
        domain.toCudaAABB()
    );

    // Build Density Hex32Field
    Hex32Field densityField;
    densityField.resolution = make_int3(
    settings.resolution.x,
    settings.resolution.y,
    settings.resolution.z
    );

    cudaMalloc(&(densityField.d_cells), hexField.resolution.x * hexField.resolution.y * hexField.resolution.z * sizeof(Hex32Cell));

    launchCalcDensityMapKernel(
        hexField,
        densityField,
        domain.toCudaAABB(),
        1000,
        0.05
    );

    // Fill 3d texture
    std::vector<Hex32Cell> hex32cells(densityField.resolution.x * densityField.resolution.y * densityField.resolution.z);
    cudaMemcpy(hex32cells.data(), densityField.d_cells, densityField.resolution.x * densityField.resolution.y * densityField.resolution.z * sizeof(Hex32Cell), cudaMemcpyDeviceToHost);

    std::vector<float> sdfData;
    sdfData.reserve(densityField.resolution.x * densityField.resolution.y * densityField.resolution.z);

    for (auto cell : hex32cells) {
        // float value = cell.nodeGradients[2].x;
        float value = cell.nodes[1];
        sdfData.push_back(value);
    }

    glBindTexture(GL_TEXTURE_3D, tex3D);
    glTexSubImage3D(
        GL_TEXTURE_3D,
        0,
        0, 0, 0,
        hexField.resolution.x,
        hexField.resolution.y,
        hexField.resolution.z,
        GL_RED,
        GL_FLOAT,
        sdfData.data()
    );

    // Cleanup
    cudaFree(d_triangles);
    cudaFree(hexField.d_cells);

    return densityField;

    // cudaDeviceSynchronize();

    // Fill 3d texture
    // std::vector<Hex32Cell> hex32cells(hexField.resolution.x * hexField.resolution.y * hexField.resolution.z);
    // cudaMemcpy(hex32cells.data(), hexField.d_cells, hexField.resolution.x * hexField.resolution.y * hexField.resolution.z * sizeof(Hex32Cell), cudaMemcpyDeviceToHost);
    //
    // std::vector<float> sdfData;
    // sdfData.reserve(hexField.resolution.x * hexField.resolution.y * hexField.resolution.z);
    //
    // for (auto cell : hex32cells) {
    //     sdfData.push_back(cell.nodes[16]);
    // }
    //
    // glBindTexture(GL_TEXTURE_3D, tex3D);
    // glTexSubImage3D(
    //     GL_TEXTURE_3D,
    //     0,
    //     0, 0, 0,
    //     hexField.resolution.x,
    //     hexField.resolution.y,
    //     hexField.resolution.z,
    //     GL_RED,
    //     GL_FLOAT,
    //     sdfData.data()
    // );
}

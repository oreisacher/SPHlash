//
// Created by ruut on 4/27/26.
//

#ifndef SPHLASH_FLUIDSURFACEMESHER_H
#define SPHLASH_FLUIDSURFACEMESHER_H

#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulation.h"

struct MesherSettings {
    glm::ivec3 resolution = { 128, 128, 128 };
    float isoLevel = 0.00f;
    bool smoothNormals = true;

    glm::vec3 color = glm::vec3(0.2f, 0.2f, 0.9f);
};

class FluidSurfaceMesher {
public:
    FluidSurfaceMesher(Scene& scene);
    ~FluidSurfaceMesher();

    void generateMesh(bool force = false) const;
    void updateVoxelFieldSize();

    void updateSettings();

    MesherSettings settings;
private:
    FluidRenderer* renderer;
    FluidSimulation* simulation;

    Model* surfaceModel;
    Mesh* surfaceMesh;

    VoxelField voxelField;

    // --- Marching Cubes ---
    int* d_vertexCount;
    int* d_triangleCount;
};

#endif //SPHLASH_FLUIDSURFACEMESHER_H

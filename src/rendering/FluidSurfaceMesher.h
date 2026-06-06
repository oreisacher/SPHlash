//
// Created by ruut on 4/27/26.
//

#ifndef SPHLASH_FLUIDSURFACEMESHER_H
#define SPHLASH_FLUIDSURFACEMESHER_H

#include <memory>
#include "Model.h"
#include "glm/glm.hpp"
#include "utils/VoxelField.h"

class Mesh;
class ModelNode;
class Scene;
class Renderer;
class FluidSimulation;

struct MesherSettings {
    glm::ivec3 resolution = { 128, 128, 128 };
    float isoLevel = 0.00f;
    bool smoothNormals = true;

    glm::vec4 color = glm::vec4(0.2f, 0.2f, 0.9f, 1.0f);
};

class FluidSurfaceMesher {
public:
    FluidSurfaceMesher(FluidSimulation* simulation);
    ~FluidSurfaceMesher();

    void updateMesh() const;
    void updateVoxelFieldSize();

    Model* getSurfaceModel() const;

    void updateSettings() const;

    MesherSettings settings;
private:
    FluidSimulation* simulation;

    Mesh* surfaceMesh;
    std::shared_ptr<Model> surfaceModel;

    VoxelField voxelField;

    // --- Marching Cubes ---
    int* d_vertexCount;
    int* d_triangleCount;
};

#endif //SPHLASH_FLUIDSURFACEMESHER_H

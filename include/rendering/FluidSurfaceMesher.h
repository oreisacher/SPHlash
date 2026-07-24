//
// Created by ruut on 4/27/26.
//

#ifndef SPHLASH_FLUIDSURFACEMESHER_H
#define SPHLASH_FLUIDSURFACEMESHER_H

#include <memory>
#include "../assets/Model.h"
#include "glm/glm.hpp"
#include "../simulation/VoxelField.h"

class Mesh;
class ModelNode;
class Scene;
class Renderer;
class FluidSimulation;

struct MesherSettings {
    glm::ivec3 resolution = { 128, 128, 128 };
    float isoLevel = 0.00f;
    bool smoothNormals = true;

    // Rendering
    glm::vec3 fluidColor    = glm::vec3(0.2f, 0.2f, 0.9f);
    float ambientStrength   = 0.3f;
    float specularStrength  = 1.0f;
    float fresnelStrength   = 0.1f;
};

class FluidSurfaceMesher {
public:
    FluidSurfaceMesher(FluidSimulation* simulation);
    ~FluidSurfaceMesher();

    void updateMesh(const AABB& domain) const;
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

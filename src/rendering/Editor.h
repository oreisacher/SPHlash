//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H
#include "ForceModel.h"
#include "Scene.h"
#include "simulation/BoundaryBuilder.h"
#include "simulation/FluidSimulation.h"

class Editor {
public:
    Editor(Scene& scene, FluidSimulation* sim);

    void updateSpawnAreaTransform();
    void updateDomainAreaTransform();

    void addForceField() const;
    void removeForceField(ForceModel* model) const;

    void addBoundaryModel(std::unique_ptr<Model> model) const;

    BoundaryBuilder* getBoundaryBuilder();

    void buildBoundaryDensityMap() const;
    void clearBoundaryDensityMap() const;
    Hex32Field getBoundaryDensityMap() const;

private:
    Scene* scene;
    FluidSimulation* simulation;
    BoundaryBuilder boundryBuilder;

    // --- Editor Objects ---
    Model* spawnAreaModel       = nullptr;
    Model* domainAreaModel      = nullptr;

    glm::mat4 calculateModelMatrix(glm::vec3 center, glm::vec3 scale);
};

#endif //SPHLASH_EDITOR_H

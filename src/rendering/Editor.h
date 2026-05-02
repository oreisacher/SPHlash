//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H
#include "Scene.h"
#include "simulation/FluidSimulation.h"

class Editor {
public:
    Editor(Scene& scene, FluidSimulation* sim);

    void updateSpawnAreaTransform();
    void updateDomainAreaTransform();

private:
    Scene* scene;
    FluidSimulation* simulation;

    // --- Editor Objects ---
    Model* spawnAreaModel       = nullptr;
    Model* domainAreaModel      = nullptr;

    void createSpawnAreaModel();
    void createSimulationDomainModel();

    glm::mat4 calculateModelMatrix(glm::vec3 center, glm::vec3 scale);
};

#endif //SPHLASH_EDITOR_H

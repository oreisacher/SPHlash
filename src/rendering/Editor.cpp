//
// Created by ruut on 4/29/26.
//

#include "Editor.h"
#include "utils/Primitives.h"
#include <memory>

Editor::Editor(Scene& scene, FluidSimulation* sim)
    : scene(&scene), simulation(sim)
{
    // Setup basic editor objects
    createSpawnAreaModel();
    createSimulationDomainModel();
}

void Editor::updateSpawnAreaTransform() {
    if (!spawnAreaModel)
        return;

    spawnAreaModel->setTransform(calculateModelMatrix(
        simulation->initSettings.spawnArea.center, simulation->initSettings.spawnArea.size
    ));
}

void Editor::updateDomainAreaTransform() {
    if (!domainAreaModel)
        return;

    domainAreaModel->setTransform(calculateModelMatrix(
        simulation->settings.domainArea.center, simulation->settings.domainArea.size
    ));
}

void Editor::createSpawnAreaModel() {
    auto spawnAreaModel = Model::createCubeModel("Particle Spawn Area");
    this->spawnAreaModel = spawnAreaModel.get();

    Material boxMaterial("Wireframe", Shader::createBox());
    boxMaterial.setVec3("color", glm::vec3(1.0f, 0.3f, 0.3f));
    spawnAreaModel->addMaterial(boxMaterial);

    scene->addModel(std::move(spawnAreaModel));

    updateSpawnAreaTransform();
}

void Editor::createSimulationDomainModel() {
    auto domainAreaModel = Model::createCubeModel("Domain Area");
    this->domainAreaModel = domainAreaModel.get();

    Material boxMaterial("Wireframe", Shader::createBox());
    boxMaterial.setVec3("color", glm::vec3(0.2f, 0.6f, 1.0f));
    domainAreaModel->addMaterial(boxMaterial);

    scene->addModel(std::move(domainAreaModel));

    updateDomainAreaTransform();
}

glm::mat4 Editor::calculateModelMatrix(glm::vec3 center, glm::vec3 scale) {
    return glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), scale);
}

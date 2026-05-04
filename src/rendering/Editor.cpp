//
// Created by ruut on 4/29/26.
//

#include "Editor.h"
#include "utils/Primitives.h"
#include <memory>

#include "AreaModel.h"
#include "ForceModel.h"

Editor::Editor(Scene& scene, FluidSimulation* sim)
    : scene(&scene), simulation(sim)
{
    // Setup basic editor objects
    // createSpawnAreaModel();
    // createSimulationDomainModel();

    auto spawnArea = std::make_unique<AreaModel>("Spawn Area", glm::vec3(1.0f, 0.3f, 0.3f), sim->initSettings.spawnArea);
    spawnAreaModel = spawnArea.get();
    scene.addModel(std::move(spawnArea));

    auto domainArea = std::make_unique<AreaModel>("Simulation Domain", glm::vec3(0.2f, 0.6f, 1.0f), sim->settings.domainArea);
    domainAreaModel = domainArea.get();
    scene.addModel(std::move(domainArea));
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

void Editor::addForceField() const {
    auto forceField = std::make_unique<ForceField>();
    auto forceFieldModel = std::make_unique<ForceModel>(forceField.get());

    forceFieldModel->addOnTransformChangedListener([this](Model *) {
        simulation->updateForceFieldBuffer();
    });

    scene->addModel(std::move(forceFieldModel));
    scene->addForceField(std::move(forceField));

    simulation->rebuildForceFieldBuffer();
}

void Editor::removeForceField(ForceModel* model) const {
    scene->removeForceField(model->getForceField());

    simulation->rebuildForceFieldBuffer();
    simulation->updateForceFieldBuffer();
}

glm::mat4 Editor::calculateModelMatrix(glm::vec3 center, glm::vec3 scale) {
    return glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), scale);
}

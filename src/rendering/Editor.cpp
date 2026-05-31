//
// Created by ruut on 4/29/26.
//

#include "Editor.h"
#include <memory>

#include "BoundingBox.h"
#include "ForceModel.h"
#include "simulation/BoundaryBuilder.h"
#include "ui/SimulationPanel.h"

Editor::Editor(Scene& scene, FluidSimulation* sim)
    : scene(&scene), simulation(sim)
{
    // Setup basic editor objects
    auto spawnArea = std::make_unique<BoundingBox>("Spawn Area", glm::vec3(1.0f, 0.3f, 0.3f));
    spawnArea->setDeletable(false);
    scene.addBoundingBox(std::move(spawnArea));

    // DEBUG Suzanne model
    addBoundaryModel(std::make_unique<Model>("assets/models/suzanne.obj"));
}

Scene* Editor::getScene() const {
    return scene;
}

// void Editor::updateSpawnAreaTransform() {
//     if (!spawnAreaModel)
//         return;
//
//     spawnAreaModel->setTransform(calculateModelMatrix(
//         simulation->initSettings.spawnArea.center, simulation->initSettings.spawnArea.size
//     ));
// }

void Editor::addParticleSpawn(std::unique_ptr<ParticleSpawnModel> model) const {
    auto particleSpawn = std::make_unique<ParticleSpawn>();
}

void Editor::addForceField() const {
    // auto forceField = std::make_unique<ForceField>();
    // auto forceFieldModel = std::make_unique<ForceModel>(forceField.get());

    // forceFieldModel->addOnTransformChangedListener([this](Model *) {
        // simulation->updateForceFieldBuffer();
    // });

    // scene->addModel(std::move(forceFieldModel));
    // scene->addForceField(std::move(forceField));

    // simulation->rebuildForceFieldBuffer();
}

void Editor::removeForceField(ForceModel* model) const {
    scene->removeForceField(model->getForceField());

    simulation->rebuildForceFieldBuffer();
    simulation->updateForceFieldBuffer();
}

void Editor::addBoundaryModel(std::unique_ptr<Model> model) const {
    model->setType(Type::BOUNDARY);
    scene->addModel(std::move(model));
}

void Editor::setSelected(SceneNode* node) {
    selectedNode = node;
}

SceneNode* Editor::getSelected() const {
    return selectedNode;
}

BoundaryBuilder* Editor::getBoundaryBuilder() {
    return &boundryBuilder;
}

void Editor::buildBoundaryDensityMap() const {
    // auto sdfMap     = boundryBuilder.constructSDFMap(scene, simulation->settings.domainArea);
    // auto volumeMap  = boundryBuilder.constructVolumeMap(sdfMap, simulation->settings.domainArea);

    // simulation->clearBoundaryField();
    // simulation->setBoundaryMaps(sdfMap, volumeMap);
}

void Editor::clearBoundaryDensityMap() const {
    simulation->clearBoundaryField();
}

Hex32Field Editor::getBoundaryDensityMap() const {
    return simulation->getBoundaryField();
}

glm::mat4 Editor::calculateModelMatrix(glm::vec3 center, glm::vec3 scale) {
    return glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), scale);
}

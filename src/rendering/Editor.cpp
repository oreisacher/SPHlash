//
// Created by ruut on 4/29/26.
//

#include "Editor.h"
#include <memory>

#include "BoundingBoxNode.h"
#include "simulation/BoundaryBuilder.h"
#include "ui/SimulationPanel.h"

Editor::Editor(Scene& scene, FluidSimulation* sim)
    : scene(&scene), simulation(sim)
{
    // Setup basic editor objects
    auto spawnArea = std::make_unique<BoundingBoxNode>("Spawn Area", glm::vec3(1.0f, 0.3f, 0.3f));
    spawnArea->setDeletable(false);
    scene.addBoundingBox(std::move(spawnArea));

    // DEBUG Suzanne model
    addBoundary(std::make_unique<Model>("assets/models/suzanne.obj"));
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
    auto forceFieldNode = std::make_unique<ForceFieldNode>("Force Field");
    scene->addForceField(std::move(forceFieldNode));
    simulation->rebuildForceFieldBuffer();

    // forceFieldModel->addOnTransformChangedListener([this](Model *) {
        // simulation->updateForceFieldBuffer();
    // });

    // scene->addModel(std::move(forceFieldModel));
    // scene->addForceField(std::move(forceField));

    // simulation->rebuildForceFieldBuffer();
}

void Editor::removeForceField(ForceFieldNode* forceFieldNode) const {
    scene->removeForceField(forceFieldNode);
    simulation->rebuildForceFieldBuffer();

    // simulation->rebuildForceFieldBuffer();
    // simulation->updateForceFieldBuffer();
}

void Editor::addBoundary(std::unique_ptr<Model> boundary) const {
    scene->addBoundary(std::move(boundary));
}

void Editor::removeBoundary(Model* boundary) const {
    scene->removeBoundary(boundary);
}

void Editor::setSelected(SceneNode* node) {
    selectedNode = node;
}

SceneNode* Editor::getSelected() const {
    return selectedNode;
}

void Editor::removeSceneNode(SceneNode* node) const {
    if (auto boundary = dynamic_cast<Model*>(node))
        removeBoundary(boundary);

    if (auto forceField = dynamic_cast<ForceFieldNode*>(node))
        removeForceField(forceField);
}

BoundaryBuilder* Editor::getBoundaryBuilder() {
    return &boundryBuilder;
}

void Editor::buildBoundaryDensityMap() const {
    auto sdfMap     = boundryBuilder.constructSDFMap(
        scene,
        simulation->getDomain()->getCudaAABB(),
        simulation->settings.h
    );

    auto volumeMap  = boundryBuilder.constructVolumeMap(
        sdfMap,
        simulation->getDomain()->getCudaAABB(),
        simulation->settings.h
    );

    simulation->clearBoundaryField();
    simulation->setBoundaryMaps(sdfMap, volumeMap);
}

void Editor::clearBoundaryDensityMap() const {
    simulation->clearBoundaryField();
}

Hex32Field Editor::getBoundaryDensityMap() const {
    return simulation->getBoundaryField();
}
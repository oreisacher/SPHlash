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
    scene.addSceneNode(std::move(spawnArea));

    // DEBUG Suzanne model
    auto model = std::make_shared<Model>("assets/models/suzanne.obj");
    auto modelNode = std::make_unique<ModelNode>("Suzanne", model);
    scene.addSceneNode(std::move(modelNode));
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

void Editor::addParticleSpawn(std::unique_ptr<ParticleSpawnNode> model) const {
    auto particleSpawn = std::make_unique<CudaParticleSpawn>();
}

void Editor::addForceField() const {
    auto forceFieldNode = std::make_unique<ForceFieldNode>("Force Field");
    scene->addSceneNode(std::move(forceFieldNode));
    simulation->rebuildForceFieldBuffer();
}

void Editor::removeForceField(const ForceFieldNode* forceFieldNode) const {
    scene->removeSceneNode(forceFieldNode);
    simulation->rebuildForceFieldBuffer();
}

void Editor::addBoundary(std::unique_ptr<ModelNode> boundaryNode) const {
    scene->addSceneNode(std::move(boundaryNode));
}

void Editor::addBoundary(const std::string& path) const {
    auto model = std::make_shared<Model>(path);
    auto modelNode = std::make_unique<ModelNode>(model->getName(), model);
    scene->addSceneNode(std::move(modelNode));
}

void Editor::removeBoundary(const ModelNode* boundaryNode) const {
    scene->removeSceneNode(boundaryNode);
}

void Editor::setSelected(SceneNode* node) {
    selectedNode = node;
}

SceneNode* Editor::getSelected() const {
    return selectedNode;
}

void Editor::removeSceneNode(SceneNode* node) const {
    scene->removeSceneNode(node);
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
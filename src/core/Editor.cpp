//
// Created by ruut on 4/29/26.
//

#include "core/Editor.h"
#include "../../include/simulation/scene/nodes/BoundingBoxNode.h"
#include "../../include/simulation/scene/nodes/BoundingBoxParticleSpawnNode.h"
#include "../../include/simulation/scene/nodes/ModelParticleSpawnNode.h"
#include "../../include/simulation/boundaries/BoundaryBuilder.h"
#include "../../include/ui/panels/SimulationPanel.h"
#include <memory>

Editor::Editor(Scene* scene, FluidSimulation* sim)
    : scene(scene), simulation(sim)
{ }

Scene* Editor::getScene() const {
    return scene;
}

void Editor::addForceField() {
    auto forceFieldNode = std::make_unique<ForceFieldNode>(std::string("Force Field"));

    selectedNode = forceFieldNode.get();
    scene->addSceneNode(std::move(forceFieldNode));
    simulation->rebuildForceFieldBuffer();
}

void Editor::removeForceField(const ForceFieldNode* forceFieldNode) const {
    scene->removeSceneNode(forceFieldNode);
    simulation->rebuildForceFieldBuffer();
}

void Editor::addParticleSpawn(const std::string& path) {
    if (path.empty()) {
        auto bbParticleSpawnNode = std::make_unique<BoundingBoxParticleSpawnNode>(std::string("Particle Spawn"));

        selectedNode = bbParticleSpawnNode.get();
        scene->addSceneNode(std::move(bbParticleSpawnNode));
        return;
    }

    auto model = std::make_shared<Model>(path);
    auto modelParticleSpawnNode = std::make_unique<ModelParticleSpawnNode>(std::string("Particle Spawn"), model);
    selectedNode = modelParticleSpawnNode.get();
    scene->addSceneNode(std::move(modelParticleSpawnNode));
}

void Editor::updateRenderingSize(int width, int height) const {
    for (auto* node : scene->getSceneNodes())
        node->updateSize(width, height);
}

void Editor::addBoundary(const std::string& path) {
    auto model = std::make_shared<Model>(path);
    auto modelNode = std::make_unique<ModelNode>(model->getName(), model);

    selectedNode = modelNode.get();
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
    Hex32Field sdfMap;
    if (!boundryBuilder.constructSDFMap(scene, scene->getSimulationDomain()->getWorldBoundingBox(), simulation->settings.h, sdfMap)) {
        // No Models are present > Clear current boundaries of simulation
        simulation->clearBoundaryField();
        return;
    }

    Hex32Field volumeMap;
    if (!boundryBuilder.constructVolumeMap(sdfMap, scene->getSimulationDomain()->getWorldBoundingBox(), simulation->settings.h, volumeMap)) {
        // Clear current boundaries of simulation if error occurred during construction
        simulation->clearBoundaryField();
        return;
    }

    simulation->clearBoundaryField();
    simulation->setBoundaryMaps(sdfMap, volumeMap);
}

void Editor::clearBoundaryDensityMap() const {
    simulation->clearBoundaryField();
}

Hex32Field Editor::getBoundaryDensityMap() const {
    return simulation->getBoundaryField();
}
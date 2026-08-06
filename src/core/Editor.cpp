//
// Created by ruut on 4/29/26.
//

#include "core/Editor.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"
#include "simulation/scene/nodes/BoundingBoxParticleSpawnNode.h"
#include "simulation/scene/nodes/ModelParticleSpawnNode.h"
#include "simulation/boundaries/BoundaryBuilder.h"
#include "ui/panels/SimulationPanel.h"
#include <memory>

Editor::Editor(Scene* scene)
    : scene(scene)
{ }

void Editor::init() const {
    // Build boundary map
    if (boundryBuilder.settings.buildOnInit)
        buildBoundaryDensityMap();

    // Init simulation
    scene->getNode<SimulationNode>()->initSimulation();

    // Call init function of each node
    for (const auto& node : scene->getSceneNodes())
        node->init();
}

void Editor::reset() const {
    scene->getNode<SimulationNode>()->reset();

    // Reset data of query nodes
    for (const auto& node : scene->getQueryNodes())
        node->reset();
}

Scene* Editor::getScene() const {
    return scene;
}

void Editor::addForceField() {
    auto forceFieldNode = std::make_unique<ForceFieldNode>(std::string("Force Field"));

    selectedNode = forceFieldNode.get();
    scene->addSceneNode(std::move(forceFieldNode));
    scene->getNode<SimulationNode>()->rebuildForceFieldBuffer();
}

void Editor::removeForceField(const ForceFieldNode* forceFieldNode) const {
    scene->removeSceneNode(forceFieldNode);
    scene->getNode<SimulationNode>()->rebuildForceFieldBuffer();
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

void Editor::addQuery() {
    auto queryNode = std::make_unique<QueryNode>();
    selectedNode = queryNode.get();
    scene->addSceneNode(std::move(queryNode));
}

void Editor::updateRenderingSize(int width, int height) const {
    for (auto* node : scene->getSceneNodes())
        if (auto *r = dynamic_cast<IRenderable*>(node))
            r->updateSize(width, height);
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
    auto simulation = scene->getNode<SimulationNode>();

    if (!simulation)
        return;

    Hex32Field sdfMap;
    if (!boundryBuilder.constructSDFMap(scene, scene->getSimulationDomain()->getWorldBoundingBox(), simulation->settings.h, sdfMap)) {
        // No Models are present > Clear current boundaries of simulation
        simulation->clearBoundaryField();
        return;
    }

    Hex32Field volumeMap;
    if (!boundryBuilder.constructVolumeMap(sdfMap, scene->getSimulationDomain()->getWorldBoundingBox(), simulation->settings.h, simulation->settings.kernelSettings, volumeMap)) {
        // Clear current boundaries of simulation if error occurred during construction
        simulation->clearBoundaryField();
        return;
    }

    simulation->clearBoundaryField();
    simulation->setBoundaryMaps(sdfMap, volumeMap);
}

void Editor::clearBoundaryDensityMap() const {
    scene->getNode<SimulationNode>()->clearBoundaryField();
}

Hex32Field Editor::getBoundaryDensityMap() const {
    return scene->getNode<SimulationNode>()->getBoundaryField();
}
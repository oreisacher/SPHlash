//
// Created by ruut on 4/29/26.
//

#include "Scene.h"
#include <algorithm>
#include <iostream>

#include "BoundingBoxParticleSpawnNode.h"
#include "simulation/FluidSimulation.h"

void Scene::setCamera(std::unique_ptr<Camera> camera) {
    this->camera = std::move(camera);
}

Camera* Scene::getCamera() const {
    return camera.get();
}

void Scene::addSceneNode(std::unique_ptr<SceneNode> node) {
    if (auto* model = dynamic_cast<ModelNode*>(node.get()))
        models.push_back(model);

    if (auto* boundingBox = dynamic_cast<BoundingBoxNode*>(node.get()))
        boundingBoxes.push_back(boundingBox);

    if (auto* forceField = dynamic_cast<ForceFieldNode*>(node.get()))
        forceFields.push_back(forceField);

    if (auto* particleSpawn = dynamic_cast<IParticleSpawn*>(node.get()))
        particleSpawns.push_back(particleSpawn);

    sceneNodes.push_back(std::move(node));
}

void Scene::removeSceneNode(const SceneNode* node) {
    std::erase_if(models,     [&](const ModelNode* n)       { return n == node; });
    std::erase_if(boundingBoxes,  [&](const BoundingBoxNode* n) { return n == node; });
    std::erase_if(forceFields,    [&](const ForceFieldNode* n)  { return n == node; });

    if (const auto* ps = dynamic_cast<const IParticleSpawn*>(node))
        std::erase_if(particleSpawns, [&](const IParticleSpawn* n) { return n == ps; });

    std::erase_if(sceneNodes, [&](const std::unique_ptr<SceneNode>& n) { return n.get() == node; });
}

std::vector<ModelNode*> Scene::getModels() const {
    return models;
}

std::vector<BoundingBoxNode*> Scene::getBoundingBoxes() const {
    return boundingBoxes;
}

std::vector<ForceFieldNode*> Scene::getForceFields() const {
    return forceFields;
}

std::vector<IParticleSpawn*> Scene::getParticleSpawns() const {
    return particleSpawns;
}

std::vector<SceneNode*> Scene::getSceneNodes() const {
    std::vector<SceneNode*> result;

    for (auto& node : sceneNodes)
        result.push_back(node.get());

    return result;
}

void Scene::saveScene(const std::string& path) const {
    Config config;

    // Simulation domain
    // Note: Domain is always the first element in list. Therefore, scene does not need to know about simulation itself.
    nlohmann::json simulationDomain;
    AABB simulationBoundingBox = boundingBoxes[0]->getWorldBoundingBox();
    simulationDomain["center"] = { simulationBoundingBox.center.x, simulationBoundingBox.center.y, simulationBoundingBox.center.z };
    simulationDomain["size"] = { simulationBoundingBox.size.x, simulationBoundingBox.size.y, simulationBoundingBox.size.z };
    config.set<nlohmann::json>("simulationDomain", simulationDomain);

    // Nodes
    nlohmann::json nodes = nlohmann::json::array();
    for (int i = 1 ; i < sceneNodes.size(); i++) {
        if (!sceneNodes[i]->isSavable())
            continue;

        nodes.push_back(sceneNodes[i]->toJson());
    }

    config.set<nlohmann::json>("nodes", nodes);
    config.save("./scene.json");
}

void Scene::loadScene(const std::string& path) {
    Config config;

    if (config.load(path)) {
        // Simulation domain
        auto simulationDomain = config.get<nlohmann::json>("simulationDomain", {});
        boundingBoxes[0]->setPosition(glm::vec3(
            simulationDomain["center"][0].get<float>(),
            simulationDomain["center"][1].get<float>(),
            simulationDomain["center"][2].get<float>()
        ));
        boundingBoxes[0]->setScale(glm::vec3(
            simulationDomain["size"][0].get<float>(),
            simulationDomain["size"][1].get<float>(),
            simulationDomain["size"][2].get<float>()
        ));

        for (auto &node : config.get<nlohmann::json>("nodes", {}))
            addSceneNode(nodeFromJson(node));
    }
}

std::unique_ptr<SceneNode> Scene::nodeFromJson(const nlohmann::json& j) {
    const std::string type = j.at("type");

    if (type == "ModelNode")
        return std::make_unique<ModelNode>(j);

    if (type == "BoundingBoxNode")
        return std::make_unique<BoundingBoxNode>(j);

    if (type == "ForceFieldNode")
        return std::make_unique<ForceFieldNode>(j);

    if (type == "BoundingBoxParticleSpawnNode")
        return std::make_unique<BoundingBoxParticleSpawnNode>(j);

    throw std::runtime_error("Unknown node type: " + type);
}

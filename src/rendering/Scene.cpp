//
// Created by ruut on 4/29/26.
//

#include "Scene.h"
#include <algorithm>
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


// void Scene::addModel(std::unique_ptr<Model> model) {
//     // models.push_back(std::move(model));
//     sceneNodes.push_back(std::move(model));
// }
//
// void Scene::removeModel(const Model* model) {
//     std::erase_if(
//         sceneNodes,
//        [&](const std::unique_ptr<SceneNode>& m){
//            return m.get() == model;
//     });
// }
//
// void Scene::removeModel(const int index) {
//     if (index < 0 || index >= sceneNodes.size()) {
//         return; // or throw an exception
//     }
//
//     sceneNodes.erase(sceneNodes.begin() + index);
// }
//
// void Scene::removeModel(const std::string& name) {
//     std::erase_if(
//         models,
//        [&](const std::unique_ptr<Model>& m){
//            return m.get()->getName() == name;
//     });
// }
//
// std::vector<Model*> Scene::getModels() const {
//     std::vector<Model*> result;
//     result.reserve(models.size());
//
//     for (auto& m : models)
//         result.push_back(m.get());
//
//     return result;
// }
//
// void Scene::addBoundary(std::unique_ptr<Model> boundary) {
//     boundaries.push_back(std::move(boundary));
// }
//
// void Scene::removeBoundary(const Model* boundary) {
//     std::erase_if(
//         boundaries,
//        [&](const std::unique_ptr<Model>& m){
//            return m.get() == boundary;
//     });
// }
//
// std::vector<Model*> Scene::getBoundaries() const {
//     std::vector<Model*> result;
//     result.reserve(boundaries.size());
//
//     for (auto& b : boundaries)
//         result.push_back(b.get());
//
//     return result;
// }
//
// void Scene::addBoundingBox(std::unique_ptr<BoundingBoxNode> boundingBox) {
//     boundingBoxes.push_back(std::move(boundingBox));
// }
//
// void Scene::removeBoundingBox(const BoundingBoxNode* model) {
//     std::erase_if(
//         boundingBoxes,
//        [&](const std::unique_ptr<BoundingBoxNode>& m){
//            return m.get() == model;
//     });
// }
//
// std::vector<BoundingBoxNode*> Scene::getBoundingBoxes() const {
//     std::vector<BoundingBoxNode*> result;
//     result.reserve(boundingBoxes.size());
//
//     for (auto& bb : boundingBoxes)
//         result.push_back(bb.get());
//
//     return result;
// }
//
// void Scene::addForceField(std::unique_ptr<ForceFieldNode> forceField) {
//     forceFields.push_back(std::move(forceField));
// }
//
// void Scene::removeForceField(const ForceFieldNode* forceField) {
//     std::erase_if(forceFields, [forceField] (const auto& m){ return m.get() == forceField; });
// }
//
// std::vector<ForceFieldNode*> Scene::getForceFields() const {
//     std::vector<ForceFieldNode*> result;
//     result.reserve(forceFields.size());
//     for (auto& m : forceFields)
//         result.push_back(m.get());
//     return result;
// }
//
// void Scene::addParticleSpawn(std::unique_ptr<IParticleSpawn> particleSpawn) {
//     particleSpawns.push_back(std::move(particleSpawn));
// }
//
// void Scene::removeParticleSpawn(const IParticleSpawn* particleSpawn) {
//     std::erase_if(particleSpawns, [particleSpawn] (const auto& m){ return m.get() == particleSpawn; });
// }

std::vector<IParticleSpawn*> Scene::getParticleSpawns() const {
    return particleSpawns;
}

// std::vector<std::pair<SceneNode*, IRenderable*>> Scene::getRenderables() const {
//     std::vector<std::pair<SceneNode*, IRenderable*>> renderables;
//
//     for (auto& node : sceneNodes) {
//         if (auto* r = dynamic_cast<IRenderable*>(node.get()))
//             renderables.push_back({ node.get(), r });
//     }
//
//     return renderables;
//
//     // for (auto& model : models)
//     //     renderables.push_back({ model.get(), model.get() });
//     //
//     // for (auto& boundary : boundaries)
//     //     renderables.push_back({ boundary.get(), boundary.get() });
//     //
//     // for (auto& boundingBox : boundingBoxes)
//     //     renderables.push_back({ boundingBox.get(), boundingBox.get() });
//     //
//     // for (auto& forceField : forceFields)
//     //     renderables.push_back({ forceField.get(), forceField.get() });
//     //
//     // for (auto& particleSpawn : particleSpawns)
//     //     renderables.push_back({ particleSpawn.get(), particleSpawn.get() });
//     //
//     // return renderables;
//     // return {};
// }

std::vector<SceneNode*> Scene::getSceneNodes() const {
    std::vector<SceneNode*> result;

    for (auto& node : sceneNodes)
        result.push_back(node.get());

    // for (auto& boundingBox : boundingBoxes)
    //     sceneNodes.push_back(boundingBox.get());
    //
    // for (auto& model : models)
    //     sceneNodes.push_back(model.get());
    //
    // for (auto& boundary : boundaries)
    //     sceneNodes.push_back(boundary.get());
    //
    // for (auto& forceField : forceFields)
    //     sceneNodes.push_back(forceField.get());
    //
    // return sceneNodes;
    return result;
}

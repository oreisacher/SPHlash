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

void Scene::addModel(std::unique_ptr<Model> model) {
    models.push_back(std::move(model));
}

void Scene::removeModel(const Model* model) {
    std::erase_if(
        models,
       [&](const std::unique_ptr<Model>& m){
           return m.get() == model;
    });
}

void Scene::removeModel(const int index) {
    if (index < 0 || index >= models.size()) {
        return; // or throw an exception
    }

    models.erase(models.begin() + index);
}

void Scene::removeModel(const std::string& name) {
    std::erase_if(
        models,
       [&](const std::unique_ptr<Model>& m){
           return m.get()->getName() == name;
    });
}

std::vector<Model*> Scene::getModels() const {
    std::vector<Model*> result;
    result.reserve(models.size());

    for (auto& m : models)
        result.push_back(m.get());

    return result;
}

void Scene::addBoundingBox(std::unique_ptr<BoundingBox> boundingBox) {
    boundingBoxes.push_back(std::move(boundingBox));
}

std::vector<BoundingBox*> Scene::getBoundingBoxes() const {
    std::vector<BoundingBox*> result;
    result.reserve(boundingBoxes.size());

    for (auto& bb : boundingBoxes)
        result.push_back(bb.get());

    return result;
}

void Scene::addForceField(std::unique_ptr<ForceField> forceField) {
    forceFields.push_back(std::move(forceField));
}

void Scene::removeForceField(const ForceField* forceField) {
    std::erase_if(forceFields, [forceField] (const auto& m){ return m.get() == forceField; });
}

std::vector<ForceField> Scene::getForceFields() const {
    std::vector<ForceField> result;
    result.reserve(forceFields.size());
    for (auto& m : forceFields)
        result.push_back(*m);
    return result;
}

std::vector<std::pair<SceneNode*, IRenderable*>> Scene::getRenderables() const {
    std::vector<std::pair<SceneNode*, IRenderable*>> renderables;

    for (auto& model : models)
        renderables.push_back({ model.get(), model.get() });

    for (auto& boundary : boundaries)
        renderables.push_back({ boundary.get(), boundary.get() });

    for (auto& boundingBox : boundingBoxes)
        renderables.push_back({ boundingBox.get(), boundingBox.get() });

    return renderables;
}

std::vector<SceneNode*> Scene::getSceneNodes() const {
    std::vector<SceneNode*> sceneNodes;

    for (auto& model : models)
        sceneNodes.push_back(model.get());

    for (auto& boundary : boundaries)
        sceneNodes.push_back(boundary.get());

    for (auto& boundingBox : boundingBoxes)
        sceneNodes.push_back(boundingBox.get());

    return sceneNodes;
}

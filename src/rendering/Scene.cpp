//
// Created by ruut on 4/29/26.
//

#include "Scene.h"

#include <algorithm>

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

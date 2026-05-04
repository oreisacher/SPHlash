//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_SCENE_H
#define SPHLASH_SCENE_H

#include <memory>
#include "Model.h"
#include "simulation/FluidSimulation.h"
#include "utils/Camera.h"

class Scene {
public:
    void setCamera(std::unique_ptr<Camera> camera);
    Camera* getCamera() const;

    void addModel(std::unique_ptr<Model> model);
    void removeModel(const Model* model);
    void removeModel(int index);
    void removeModel(const std::string& name);

    std::vector<Model*> getModels() const;

    void addForceField(std::unique_ptr<ForceField> forceField);
    void removeForceField(const ForceField* forceField);
    std::vector<ForceField> getForceFields() const;

private:
    std::unique_ptr<Camera> camera                                  = nullptr;
    std::vector<std::unique_ptr<Model>> models                      = {};
    std::vector<std::unique_ptr<ForceField>> forceFields            = {};
};

#endif //SPHLASH_SCENE_H

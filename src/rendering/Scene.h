//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_SCENE_H
#define SPHLASH_SCENE_H

#include <memory>

#include "BoundingBox.h"
#include "Model.h"
#include "simulation/FluidSimulation.h"
#include "utils/Camera.h"

class Scene {
public:
    // --- Camera ---
    Camera* getCamera() const;
    void setCamera(std::unique_ptr<Camera> camera);

    void addModel(std::unique_ptr<Model> model);
    void removeModel(const Model* model);
    void removeModel(int index);
    void removeModel(const std::string& name);

    std::vector<Model*> getModels() const;

    // --- Bounding Boxes ---
    void addBoundingBox(std::unique_ptr<BoundingBox> boundingBox);
    std::vector<BoundingBox*> getBoundingBoxes() const;

    // --- Force Fields ---
    void addForceField(std::unique_ptr<ForceField> forceField);
    void removeForceField(const ForceField* forceField);
    std::vector<ForceField> getForceFields() const;

    std::vector<std::pair<SceneNode*, IRenderable*>> getRenderables() const;
    std::vector<SceneNode*> getSceneNodes() const;

private:
    std::unique_ptr<Camera> camera                                  = nullptr;

    std::vector<std::unique_ptr<Model>>         models              = {};
    std::vector<std::unique_ptr<Model>>         boundaries          = {};
    std::vector<std::unique_ptr<BoundingBox>>   boundingBoxes       = {};
    std::vector<std::unique_ptr<ForceField>>    forceFields         = {};
};

#endif //SPHLASH_SCENE_H

//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_SCENE_H
#define SPHLASH_SCENE_H

#include <memory>

#include "BoundingBoxNode.h"
#include "ForceFieldNode.h"
#include "Model.h"
#include "simulation/FluidSimulation.h"
#include "simulation/IParticleSpawn.h"
#include "utils/Camera.h"

class Scene {
public:
    // --- Camera ---
    Camera* getCamera() const;
    void setCamera(std::unique_ptr<Camera> camera);

    void addSceneNode(std::unique_ptr<SceneNode> node);
    void removeSceneNode(const SceneNode* node);

    // // --- Models ---
    // void addModel(std::unique_ptr<Model> model);
    // void removeModel(const Model* model);
    // void removeModel(int index);
    // void removeModel(const std::string& name);
    // std::vector<Model*> getModels() const;
    //
    // // --- Boundary ---
    // void addBoundary(std::unique_ptr<Model> boundary);
    // void removeBoundary(const Model* boundary);
    std::vector<ModelNode*> getModels() const;
    //
    // // --- Bounding Boxes ---
    // void addBoundingBox(std::unique_ptr<BoundingBoxNode> boundingBox);
    // void removeBoundingBox(const BoundingBoxNode* model);
    std::vector<BoundingBoxNode*> getBoundingBoxes() const;
    //
    // // --- Force Fields ---
    // void addForceField(std::unique_ptr<ForceFieldNode> forceField);
    // void removeForceField(const ForceFieldNode* forceField);
    std::vector<ForceFieldNode*> getForceFields() const;

    // // --- Particle Spawns ---
    // void addParticleSpawn(std::unique_ptr<IParticleSpawn> particleSpawn);
    // void removeParticleSpawn(const IParticleSpawn* particleSpawn);
    std::vector<IParticleSpawn*> getParticleSpawns() const;

    std::vector<SceneNode*> getSceneNodes() const;

private:
    std::unique_ptr<Camera> camera                                  = nullptr;

    std::vector<std::unique_ptr<SceneNode>> sceneNodes                 = {};

    // std::vector<std::unique_ptr<Model>>             models              = {};
    std::vector<ModelNode*>         models              = {};
    std::vector<BoundingBoxNode*>   boundingBoxes       = {};
    std::vector<ForceFieldNode*>    forceFields         = {};
    std::vector<IParticleSpawn*>    particleSpawns      = {};
};

#endif //SPHLASH_SCENE_H

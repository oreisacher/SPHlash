//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_SCENE_H
#define SPHLASH_SCENE_H

#include <vector>
#include <nlohmann/json_fwd.hpp>
#include "utils/Camera.h"

class IParticleSpawn;
class ForceFieldNode;
class ModelNode;
class ParticlesNode;
class BoundingBoxNode;
class SceneNode;

class Scene {
public:
    // --- Camera ---
    Camera* getCamera() const;
    void setCamera(std::unique_ptr<Camera> camera);

    void addSceneNode(std::unique_ptr<SceneNode> node);
    void removeSceneNode(const SceneNode* node);

    void setSimulationDomain(BoundingBoxNode* domain);
    BoundingBoxNode* getSimulationDomain() const;
    void setParticleNode(ParticlesNode* particles);
    ParticlesNode* getParticlesNode() const;

    std::vector<ModelNode*> getModels() const;
    std::vector<BoundingBoxNode*> getBoundingBoxes() const;
    std::vector<ForceFieldNode*> getForceFields() const;
    std::vector<IParticleSpawn*> getParticleSpawns() const;
    std::vector<SceneNode*> getSceneNodes() const;

    void saveScene(const std::string& path) const;
    void loadScene(const std::string& path);
    void resetScene();

private:
    std::unique_ptr<Camera> camera                      = nullptr;

    // --- Scene Nodes ---
    std::vector<std::unique_ptr<SceneNode>> sceneNodes  = {};

    // --- Node Cache ---
    BoundingBoxNode*                simulationDomain    = nullptr;
    ParticlesNode*                  particles           = nullptr;
    std::vector<ModelNode*>         models              = {};
    std::vector<BoundingBoxNode*>   boundingBoxes       = {};
    std::vector<ForceFieldNode*>    forceFields         = {};
    std::vector<IParticleSpawn*>    particleSpawns      = {};

    std::unique_ptr<SceneNode> nodeFromJson(const nlohmann::json& j);
};

#endif //SPHLASH_SCENE_H

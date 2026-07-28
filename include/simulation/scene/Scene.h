//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_SCENE_H
#define SPHLASH_SCENE_H

#include <vector>
#include <nlohmann/json_fwd.hpp>
#include "nodes/CameraNode.h"
#include "nodes/QueryNode.h"

class IParticleSpawn;
class ForceFieldNode;
class ModelNode;
class ParticlesNode;
class BoundingBoxNode;
class SceneNode;

class Scene {
public:
    Scene(Window* window);

    void update(float deltaTime) const;

    void addSceneNode(std::unique_ptr<SceneNode> node);
    void removeSceneNode(const SceneNode* node);

    CameraNode* getCamera() const;

    BoundingBoxNode* getSimulationDomain() const;
    ParticlesNode* getParticlesNode() const;

    std::vector<ModelNode*> getModels() const;
    std::vector<BoundingBoxNode*> getBoundingBoxes() const;
    std::vector<ForceFieldNode*> getForceFields() const;
    std::vector<IParticleSpawn*> getParticleSpawns() const;
    std::vector<SceneNode*> getSceneNodes() const;
    std::vector<QueryNode*> getQueryNodes() const;

    // Returns the first node of type T
    template<typename T>
    T* getNode() {
        for (auto node : getSceneNodes()) {
            if (dynamic_cast<T*>(node)) {
                return dynamic_cast<T*>(node);
            }
        }

        return nullptr;
    }

    // Returns the all node of type T
    template<typename T>
    std::vector<T*> getNodes() {
        std::vector<T*> nodes;

        for (auto node : getSceneNodes())
            if (auto n = dynamic_cast<T*>(node))
                nodes.push_back(n);

        return nodes;
    }

    void saveScene(const std::string& path) const;
    void loadScene(const std::string& path);
    void resetScene();

private:

    // --- Scene Nodes ---
    std::vector<std::unique_ptr<SceneNode>> sceneNodes  = {};

    // --- Node Cache ---
    SimulationNode*            simulationNode      = nullptr;
    CameraNode*                     camera              = nullptr;
    BoundingBoxNode*                simulationDomain    = nullptr;
    ParticlesNode*                  particles           = nullptr;
    std::vector<ModelNode*>         models              = {};
    std::vector<BoundingBoxNode*>   boundingBoxes       = {};
    std::vector<ForceFieldNode*>    forceFields         = {};
    std::vector<IParticleSpawn*>    particleSpawns      = {};
    std::vector<QueryNode*>         queryNodes          = {};

    std::unique_ptr<SceneNode> nodeFromJson(const nlohmann::json& j);
};

#endif //SPHLASH_SCENE_H

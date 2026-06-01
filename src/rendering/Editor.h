//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H

#include "ForceFieldNode.h"
#include "ParticleSpawnNode.h"
#include "Scene.h"
#include "simulation/BoundaryBuilder.h"
#include "simulation/FluidSimulation.h"

class Editor {
public:
    Editor(Scene& scene, FluidSimulation* sim);

    Scene* getScene() const;

    void setSelected(SceneNode* node);
    SceneNode* getSelected() const;

    void removeSceneNode(SceneNode* node) const;

    // --- Boundary ---
    void addBoundary(std::unique_ptr<ModelNode> boundary) const;
    void addBoundary(const std::string& path) const;
    void removeBoundary(const ModelNode* boundaryNode) const;

    BoundaryBuilder* getBoundaryBuilder();

    void buildBoundaryDensityMap() const;
    void clearBoundaryDensityMap() const;
    Hex32Field getBoundaryDensityMap() const;

    // --- Force Fields ---
    void addForceField() const;
    void removeForceField(const ForceFieldNode* forceFieldNode) const;

    // --- Particle Spawns ---
    void addParticleSpawn(std::unique_ptr<ParticleSpawnNode> model) const;
private:
    Scene* scene;
    FluidSimulation* simulation;
    BoundaryBuilder boundryBuilder;

    SceneNode* selectedNode = nullptr;
};

#endif //SPHLASH_EDITOR_H

//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H

#include "ForceFieldNode.h"
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
    void addBoundary(const std::string& path);
    void removeBoundary(const ModelNode* boundaryNode) const;

    BoundaryBuilder* getBoundaryBuilder();

    void buildBoundaryDensityMap() const;
    void clearBoundaryDensityMap() const;
    Hex32Field getBoundaryDensityMap() const;

    // --- Force Fields ---
    void addForceField();
    void removeForceField(const ForceFieldNode* forceFieldNode) const;

    // --- Particle Spawns ---
    void addParticleSpawn(const std::string& path);
private:
    Scene* scene;
    FluidSimulation* simulation;
    BoundaryBuilder boundryBuilder;

    SceneNode* selectedNode = nullptr;
};

#endif //SPHLASH_EDITOR_H

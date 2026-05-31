//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H

#include "ForceFieldNode.h"
#include "ParticleSpawnModel.h"
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
    void addBoundary(std::unique_ptr<Model> boundary) const;
    void removeBoundary(Model* boundary) const;

    BoundaryBuilder* getBoundaryBuilder();

    void buildBoundaryDensityMap() const;
    void clearBoundaryDensityMap() const;
    Hex32Field getBoundaryDensityMap() const;

    // --- Force Fields ---
    void addForceField() const;
    void removeForceField(ForceFieldNode* forceFieldNode) const;

    // --- Particle Spawns ---
    void addParticleSpawn(std::unique_ptr<ParticleSpawnModel> model) const;
private:
    Scene* scene;
    FluidSimulation* simulation;
    BoundaryBuilder boundryBuilder;

    SceneNode* selectedNode;
};

#endif //SPHLASH_EDITOR_H

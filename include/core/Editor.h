//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H

#include "simulation/scene/nodes/ForceFieldNode.h"
#include "simulation/scene/Scene.h"
#include "simulation/boundaries/BoundaryBuilder.h"

class Editor {
public:
    Editor(Scene* scene);

    void init() const;
    void reset() const;

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

    // --- Query Node ---
    void addQuery();

    // --- Window Resizes ---
    void updateRenderingSize(int width, int height) const;

private:
    Scene* scene;
    BoundaryBuilder boundryBuilder;

    SceneNode* selectedNode = nullptr;
};

#endif //SPHLASH_EDITOR_H

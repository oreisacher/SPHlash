//
// Created by ruut on 4/29/26.
//

#include "Editor.h"
#include "utils/Primitives.h"
#include <memory>

Editor::Editor(Scene& scene) {
    this->scene = &scene;

    // Setup SpawnArea
    std::vector<std::unique_ptr<Mesh>> meshes;
    meshes.push_back(std::make_unique<Mesh>(
        "PSA",
        Primitives::BoxGeometry::GetVertices(),
        Primitives::BoxGeometry::GetWireframeIndices(),
        GL_LINES
    ));

    auto spawnAreaModel = std::make_unique<Model>(
        "Particle Spawn Area",
        std::move(meshes)
    );

    Material boxMaterial("Wireframe", Shader::createBox());
    boxMaterial.setVec3("color", glm::vec3(1.0f, 0.3f, 0.3f));
    spawnAreaModel->addMaterial(boxMaterial);

    scene.addModel(std::move(spawnAreaModel));
}

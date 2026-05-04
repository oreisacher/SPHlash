//
// Created by ruut on 5/4/26.
//

#include "AreaModel.h"

AreaModel::AreaModel(const std::string& name, glm::vec3 color, AABB& area) : Model(name, nullptr), area(&area) {
    addMesh(Mesh::createWireframeBox(name + " Mesh"));

    Material boxMaterial("Wireframe", Shader::createBox());
    boxMaterial.setVec3("color", color);
    addMaterial(boxMaterial);

    // Update models transform with area data
    transform.position = area.center;
    transform.scale = area.size;
    updateTransform();

    sceneGraphDeletable = false;

    addOnTransformChangedListener([this](Model* model) {
        this->area->setCenter(transform.position);
        this->area->setSize(transform.scale);
    });
}
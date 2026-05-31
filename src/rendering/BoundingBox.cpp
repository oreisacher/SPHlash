//
// Created by ruut on 5/4/26.
//

#include "BoundingBox.h"

BoundingBox::BoundingBox(const std::string& name, glm::vec3 color) {
    this->name = name;

    mesh = Mesh::createWireframeBox(name);
    material = std::make_unique<Material>("Wireframe", Shader::createBox());
    material->setVec3("color", color);
}

void BoundingBox::draw(const glm::mat4& view, const glm::mat4& projection) const {
    // Setup wireframe shader
    material->bind();
    material->getShader()->setMat4("view", view);
    material->getShader()->setMat4("projection", projection);
    material->getShader()->setMat4("model", transform);

    mesh->draw();
}

void BoundingBox::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);

    // Force rotation to zeros
    setRotation({0.0, 0.0, 0.0, 0.0});
}

AABB BoundingBox::getAABB() const {
    return {
        position,
        scale
    };
}

CudaAABB BoundingBox::getCudaAABB() const {
    return getAABB().toCudaAABB();
}

Properties BoundingBox::buildProperties() {
    Properties properties;

    properties.addSection("Transform");
    properties.addProperty(std::make_unique<Vec3Property>(
        "Position",
        [this] () { return getPosition(); },
        [this] (glm::vec3 position) { setPosition(position); }
    ));
    properties.addProperty(std::make_unique<Vec3Property>(
        "Scale",
        [this] () { return getScale(); },
        [this] (glm::vec3 scale) { setScale(scale); }
    ));

    return properties;
}

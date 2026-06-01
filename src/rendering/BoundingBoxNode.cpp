//
// Created by ruut on 5/4/26.
//

#include "BoundingBoxNode.h"

BoundingBoxNode::BoundingBoxNode(const std::string& name, glm::vec3 color) {
    this->name = name;

    mesh = Mesh::createWireframeBox(name);
    material = std::make_unique<Material>("Wireframe", Shader::createBox());
    material->setVec3("color", color);
}

void BoundingBoxNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    // Setup wireframe shader
    material->bind();
    material->getShader()->setMat4("view", view);
    material->getShader()->setMat4("projection", projection);

    material->getShader()->setMat4("model", target ? target->getWorldBoundingBox().getTransformMatrix() : transform );

    mesh->draw();
}

void BoundingBoxNode::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);

    // Force rotation to zeros
    setRotation({0.0, 0.0, 0.0, 0.0});
}

void BoundingBoxNode::setTransform(const AABB& boundingBox) {
    position = boundingBox.center;
    rotation = {0.0, 0.0, 0.0, 0.0};
    scale = boundingBox.size;

    updateTransform();
}

void BoundingBoxNode::setTarget(IBounded* target) {
    this->target = target;
}

void BoundingBoxNode::clearTarget() {
    this->target = nullptr;
}

CudaAABB BoundingBoxNode::getCudaAABB() const {
    return getWorldBoundingBox().toCudaAABB();
}

AABB BoundingBoxNode::getWorldBoundingBox() const {
    return { position, scale };
}

Properties BoundingBoxNode::buildProperties() {
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

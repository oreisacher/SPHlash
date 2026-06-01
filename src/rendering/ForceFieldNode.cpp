//
// Created by ruut on 5/4/26.
//

#include "ForceFieldNode.h"

ForceFieldNode::ForceFieldNode(const std::string& name) {
    this->name = name;

    // Setup Force Field Arrow Model
    model = std::make_unique<Model>("assets/models/forceField.obj", false);

    ForceFieldNode::setScale({ 0.25, 0.25, 0.25 });
}

CudaForceField ForceFieldNode::getForceField() const {
    return forceField;
}

void ForceFieldNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    model->draw(view, projection, transform);
}

void ForceFieldNode::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);

    // Sync arrow model transform with this transform
    // model->setTransform(transform);

    // Update cuda force field
    glm::vec3 right   = rotation * glm::vec3(1, 0,  0);
    glm::vec3 up      = rotation * glm::vec3(0, 1,  0);
    glm::vec3 forward = rotation * glm::vec3(0, 0, -1);

    forceField.position = { position.x, position.y, position.z };
    forceField.halfExtents = { 0.5f * scale.x, 0.5f * scale.y, 0.5f * scale.z };
    forceField.right = { right.x, right.y, right.z };
    forceField.up = { up.x, up.y, up.z };
    forceField.forward = { forward.x, forward.y, forward.z };
}

void ForceFieldNode::updateTransform() {
    Transform::updateTransform();

    // Sync arrow model transform with this transform
    // model->setTransform(transform);

    // Update cuda force field
    glm::vec3 right   = rotation * glm::vec3(1, 0,  0);
    glm::vec3 up      = rotation * glm::vec3(0, 1,  0);
    glm::vec3 forward = rotation * glm::vec3(0, 0, -1);

    forceField.position = { position.x, position.y, position.z };
    forceField.halfExtents = { 0.5f * scale.x, 0.5f * scale.y, 0.5f * scale.z };
    forceField.right = { right.x, right.y, right.z };
    forceField.up = { up.x, up.y, up.z };
    forceField.forward = { forward.x, forward.y, forward.z };
}

Properties ForceFieldNode::buildProperties() {
    Properties properties;

    properties.addSection("Transform");
    properties.addProperty(std::make_unique<Vec3Property>(
        "Position",
        [this] () { return getPosition(); },
        [this] (glm::vec3 position) { setPosition(position); }
    ));
    properties.addProperty(std::make_unique<Vec3Property>(
        "Rotation",
        [this] () { return getEulerRotation(); },
        [this] (glm::vec3 rotation) { setEulerRotation(rotation); }
    ));
    properties.addProperty(std::make_unique<Vec3Property>(
        "Scale",
        [this] () { return getScale(); },
        [this] (glm::vec3 scale) { setScale(scale); }
    ));

    properties.addSection("Force Field");
    properties.addProperty(std::make_unique<FloatProperty>(
        "Strength",
        [this] () { return forceField.strength; },
        [this] (float s) { forceField.strength = s; }
    ));

    return properties;
}


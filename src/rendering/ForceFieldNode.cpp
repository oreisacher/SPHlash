//
// Created by ruut on 5/4/26.
//

#include "ForceFieldNode.h"

#include "Model.h"

ForceFieldNode::ForceFieldNode(const std::string& name) {
    this->name = name;

    // Setup Force Field Arrow Model
    model = std::make_unique<Model>("assets/models/forceField.obj", false);

    ForceFieldNode::setScale({ 0.25, 0.25, 0.25 });
}

ForceFieldNode::ForceFieldNode(const nlohmann::json json) : SceneNode(json) {
    // Transform
    position = {
        json["position"][0].get<float>(),
        json["position"][1].get<float>(),
        json["position"][2].get<float>()
    };
    rotation = {
        json["rotation"][3].get<float>(),
        json["rotation"][0].get<float>(),
        json["rotation"][1].get<float>(),
        json["rotation"][2].get<float>()
    };
    scale = {
        json["scale"][0].get<float>(),
        json["scale"][1].get<float>(),
        json["scale"][2].get<float>()
    };

    ForceFieldNode::updateTransform();

    // Forcefield
    forceField.active = json["active"];
    forceField.strength = json["strength"];

    model = std::make_unique<Model>("assets/models/forceField.obj", false);
}

CudaForceField ForceFieldNode::getForceField() const {
    return forceField;
}

void ForceFieldNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    model->draw(view, projection, transform);
}

void ForceFieldNode::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);

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
    properties.addProperty(std::make_unique<BoolProperty>(
        "Enabled",
        [this] () { return forceField.active; },
        [this] (bool on) { forceField.active = on; }
    ));
    properties.addProperty(std::make_unique<FloatProperty>(
        "Strength",
        [this] () { return forceField.strength; },
        [this] (float s) { forceField.strength = s; }
    ));

    return properties;
}

nlohmann::json ForceFieldNode::toJson() const {
    nlohmann::json j = SceneNode::toJson();

    // Transform
    j["position"] = { position.x, position.y, position.z };
    j["rotation"] = { rotation.x, rotation.y, rotation.z, rotation.w };
    j["scale"] = { scale.x, scale.y, scale.z };

    // Forcefield
    j["active"] = forceField.active;
    j["strength"] = forceField.strength;

    return j;
}

std::string ForceFieldNode::getTypeName() const {
    return "ForceFieldNode";
}


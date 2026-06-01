//
// Created by ruut on 6/1/26.
//

#include "ModelNode.h"

ModelNode::ModelNode(const std::string& name, const std::shared_ptr<Model>& model) {
    this->name = name;
    this->model = model;

    ModelNode::updateTransform();

    // Add bounding box node as child
    auto bbNode = std::make_unique<BoundingBoxNode>("Bounding Box", glm::vec3(1.0f));
    boundingBox = bbNode.get();
    boundingBox->setTarget(this);
    boundingBox->setVisible(false);
    children.push_back(std::move(bbNode));
}

Model* ModelNode::getModel() const {
    return model.get();
}

std::vector<Triangle> ModelNode::getWorldSpaceTriangles() const {
    std::vector<Triangle> triangles;

    for (const auto& mesh : model->getMeshes()) {
        auto meshTriangles = mesh->getWorldSpaceTriangles(transform);
        triangles.insert(triangles.end(), meshTriangles.begin(), meshTriangles.end());
    }

    return triangles;
}

AABB ModelNode::getBoundingBox() const {
    return model->getBoundingBox();
}

AABB ModelNode::getWorldBoundingBox() const {
    return worldBoundingBox;
}

void ModelNode::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);
    worldBoundingBox = model->getWorldBoundingBox(transform);
}

void ModelNode::updateTransform() {
    Transform::updateTransform();
    worldBoundingBox = model->getWorldBoundingBox(transform);
}

bool ModelNode::isBoundary() const {
    return boundary;
}

void ModelNode::setBoundary(bool on) {
    this->boundary = on;
}

void ModelNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    model->draw(view, projection, transform);
}

Properties ModelNode::buildProperties() {
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

    properties.addSection("Boundary");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Enable Boundary",
        [this] () { return boundary; },
        [this] (bool on) { boundary = on; }
    ));

    properties.addSection("Debug");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Show Bounding Box",
        [this] () { return boundingBox->isVisible(); },
        [this] (bool on) { boundingBox->setVisible(on); }
    ));

    return properties;
}

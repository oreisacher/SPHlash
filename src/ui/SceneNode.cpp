//
// Created by ruut on 5/31/26.
//

#include "SceneNode.h"

SceneNode::SceneNode(const nlohmann::json j) {
    this->name = j["name"];
    this->visible = j["visible"];
}

const std::string& SceneNode::getName() const {
    return name;
}

void SceneNode::setName(const std::string& name) {
    this->name = name;
}

bool SceneNode::isVisible() const {
    return visible;
}

void SceneNode::setVisible(bool visible) {
    this->visible = visible;
}

bool SceneNode::isSceneGraphVisible() const {
    return sceneGraphVisible;
}

void SceneNode::setSceneGraphVisible(bool visible) {
    this->sceneGraphVisible = visible;
}

bool SceneNode::isSavable() const {
    return savable;
}

void SceneNode::setSavable(bool savable) {
    this->savable = savable;
}

nlohmann::json SceneNode::toJson() const {
    nlohmann::json j;

    j["type"] = getTypeName();
    j["name"] = name;
    j["visible"] = visible;

    return j;
}

std::string SceneNode::getTypeName() const {
    return "SceneNode";
}

std::vector<SceneNode*> SceneNode::getChildren() const {
    std::vector<SceneNode*> result;

    for (auto &c : children)
        result.push_back(c.get());

    return result;
}

bool SceneNode::isDeletable() const {
    return deletable;
}

void SceneNode::setDeletable(bool deletable) {
    this->deletable = deletable;
}

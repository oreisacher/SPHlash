//
// Created by ruut on 7/4/26.
//

#include "simulation/scene/nodes/SimulationDomainNode.h"

SimulationDomainNode::SimulationDomainNode()
    : BoundingBoxNode("Simulation Domain", glm::vec4(0.2f, 0.6f, 1.0f, 1.0f))
{
    BoundingBoxNode::setScale({ 2.0f, 2.0f, 2.0f });
    addTag("simulation_Domain");
    setDeletable(false);
}

SimulationDomainNode::SimulationDomainNode(const nlohmann::json& json) : BoundingBoxNode(json) {
    addTag("simulation_Domain");
    setDeletable(false);
}

std::string SimulationDomainNode::getTypeName() const {
    return "SimulationDomainNode";
}

std::string SimulationDomainNode::getDefaultName() const {
    return "Simulation Domain";
}

SceneNode* SimulationDomainNode::getNode() {
    return this;
}

//
// Created by ruut on 7/4/26.
//

#ifndef SPHLASH_SIMULATIONDOMAINNODE_H
#define SPHLASH_SIMULATIONDOMAINNODE_H

#include "BoundingBoxNode.h"

class SimulationDomainNode : public BoundingBoxNode {
public:
    SimulationDomainNode();
    explicit SimulationDomainNode(const nlohmann::json& json);
    std::string getTypeName() const override;
    std::string getDefaultName() const override;
};

#endif //SPHLASH_SIMULATIONDOMAINNODE_H

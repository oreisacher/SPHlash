//
// Created by ruut on 7/24/26.
//

#ifndef SPHLASH_QUERYNODE_H
#define SPHLASH_QUERYNODE_H

#include "BoundingBoxNode.h"
#include "simulation/FluidSimulation.h"

struct Metrics {
    int count = 0;

    // Average particle across all particles inside query bb
    glm::vec3 position              = { 0.0, 0.0, 0.0 };
    glm::vec3 velocity              = { 0.0, 0.0, 0.0 };
    glm::vec3 pressureForce         = { 0.0, 0.0, 0.0 };
    glm::vec3 viscosityForce        = { 0.0, 0.0, 0.0 };
    glm::vec3 surfaceTensionForce   = { 0.0, 0.0, 0.0 };

    float density                   = 0;
    float densityError              = 0;
    float pressure                  = 0;

};

inline std::ostream& operator<<(std::ostream& os, const Metrics& m) {
    os
        << m.count << ","
        << m.position.x << "," << m.position.y << "," << m.position.z << ","
        << m.velocity.x << "," << m.velocity.y << "," << m.velocity.z << ","
        << m.pressureForce.x << "," << m.pressureForce.y << "," << m.pressureForce.z << ","
        << m.viscosityForce.x << "," << m.viscosityForce.y << "," << m.viscosityForce.z << ","
        << m.surfaceTensionForce.x << "," << m.surfaceTensionForce.y << "," << m.surfaceTensionForce.z << ","
        << m.density << ","
        << m.densityError << ","
        << m.pressure;

    return os;
}

class QueryNode : public BoundingBoxNode {
public:
    QueryNode(FluidSimulation* simulation);
    explicit QueryNode(const nlohmann::json json);

    void init() override;
    void update(float deltaTime) override;

    void reset();
    void exportData(const std::string& path);

    std::vector<Metrics>& getHistory();

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    FluidSimulation* simulation;
    int filter = 0;

    std::vector<Metrics> history;

    Metrics* d_blockMetric  = nullptr;
    Metrics* d_result       = nullptr;
    void* d_tempStorage     = nullptr;
    size_t tempStorageBytes = 0;
    int allocatedBlocks     = 0;
};

#endif //SPHLASH_QUERYNODE_H

//
// Created by ruut on 6/1/26.
//

#include "BoundingBoxParticleSpawnNode.h"

BoundingBoxParticleSpawnNode::BoundingBoxParticleSpawnNode(const std::string& name, const glm::vec3& color)
    : BoundingBoxNode(name, color)
{
    // Init particle spawn
    particleSpawn.hasShape = false;
    particleSpawn.d_shapeTriangles = nullptr;
}

CudaParticleSpawn BoundingBoxParticleSpawnNode::getParticleSpawn() {
    return particleSpawn;
}

Properties BoundingBoxParticleSpawnNode::buildProperties() {
    Properties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Particle Spawn");
    properties.addProperty(std::make_unique<FloatProperty>(
        "Total Mass",
        [this] () { return particleSpawn.totalMass; },
        [this] (float mass) { particleSpawn.totalMass = mass; }
    ));
    properties.addProperty(std::make_unique<FloatProperty>(
        "Particle Spacing",
        [this] () { return particleSpawn.particleSpacing; },
        [this] (float spacing) { particleSpawn.particleSpacing = spacing; }
    ));

    return properties;
}

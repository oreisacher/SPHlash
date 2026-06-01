//
// Created by ruut on 6/1/26.
//

#include "BoundingBoxParticleSpawnNode.h"

BoundingBoxParticleSpawnNode::BoundingBoxParticleSpawnNode(const std::string& name)
    : BoundingBoxNode(name, glm::vec3(1.0f, 0.0, 0.0))
{
    // Init particle spawn
    particleSpawn.hasShape = false;
    particleSpawn.d_shapeTriangles = nullptr;
}

BoundingBoxParticleSpawnNode::BoundingBoxParticleSpawnNode(const nlohmann::json json) : BoundingBoxNode(json) {
    // Particle spawn
    particleSpawn.restDensity = json["restDensity"];
    particleSpawn.particleSpacing = json["particleSpacing"];
    particleSpawn.hasShape = json["hasShape"];
    particleSpawn.color = make_float4(
        json["color"][0].get<float>(),
        json["color"][1].get<float>(),
        json["color"][2].get<float>(),
        json["color"][3].get<float>()
    );
}

CudaParticleSpawn BoundingBoxParticleSpawnNode::getParticleSpawn() {
    // Update bounding box
    particleSpawn.area = getWorldBoundingBox().toCudaAABB();

    return particleSpawn;
}

Properties BoundingBoxParticleSpawnNode::buildProperties() {
    Properties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Particle Spawn");
    properties.addProperty(std::make_unique<FloatProperty>(
        "Rest Density",
        [this] () { return particleSpawn.restDensity; },
        [this] (float mass) { particleSpawn.restDensity = mass; }
    ));
    properties.addProperty(std::make_unique<FloatProperty>(
        "Particle Spacing",
        [this] () { return particleSpawn.particleSpacing; },
        [this] (float spacing) { particleSpawn.particleSpacing = spacing; }
    ));
    properties.addProperty(std::make_unique<ColorProperty>(
        "Particle Color",
        [this] () { return glm::vec4(particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z, particleSpawn.color.w); },
        [this] (glm::vec4 color) { particleSpawn.color = { color.x, color.y, color.z, color.w }; }
    ));

    return properties;
}

nlohmann::json BoundingBoxParticleSpawnNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();

    j["restDensity"] = particleSpawn.restDensity;
    j["particleSpacing"] = particleSpawn.particleSpacing;
    j["color"] = { particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z, particleSpawn.color.w };
    j["hasShape"] = particleSpawn.hasShape;

    return j;
}

std::string BoundingBoxParticleSpawnNode::getTypeName() const {
    return "BoundingBoxParticleSpawnNode";
}

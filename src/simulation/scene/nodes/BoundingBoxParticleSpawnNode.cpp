//
// Created by ruut on 6/1/26.
//

#include "../../../../include/simulation/scene/nodes/BoundingBoxParticleSpawnNode.h"
#include <vector_functions.h>
#include "../../../../include/assets/Material.h"
#include "../../../../include/simulation/scene/Properties.h"

BoundingBoxParticleSpawnNode::BoundingBoxParticleSpawnNode(const std::string& name)
    : BoundingBoxNode(name, glm::vec3(1.0f, 0.0, 0.0))
{
    // Init particle spawn
    particleSpawn.hasShape = false;
    particleSpawn.d_shapeTriangles = nullptr;
}

BoundingBoxParticleSpawnNode::BoundingBoxParticleSpawnNode(const nlohmann::json json) : BoundingBoxNode(json) {
    particleSpawn.hasShape = false;
    particleSpawn.d_shapeTriangles = nullptr;

    // Particle spawn
    particleSpawn.restDensity = json.value("restDensity", particleSpawn.restDensity);
    particleSpawn.particleSpacing = json.value("particleSpacing", particleSpawn.particleSpacing);

    auto color = json.value("color", std::vector{particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z, particleSpawn.color.w });
    particleSpawn.color = make_float4(
        color[0],
        color[1],
        color[2],
        1.0f
    );

    // Update shader color variable
    material->bind();
    material->setVec3("color", {
        particleSpawn.color.x,
        particleSpawn.color.y,
        particleSpawn.color.z,
    });
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
        [this] (float mass) { particleSpawn.restDensity = mass; },
        0.0, INFINITY, 1.0f
    ));
    properties.addProperty(std::make_unique<FloatProperty>(
        "Particle Spacing",
        [this] () { return particleSpawn.particleSpacing; },
        [this] (float spacing) { particleSpawn.particleSpacing = spacing; },
        0.001f, INFINITY, 0.0001f
    ));
    properties.addProperty(std::make_unique<ColorProperty>(
        "Particle Color",
        [this] () { return glm::vec3(particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z); },
        [this] (glm::vec3 color)
        {
            particleSpawn.color = { color.x, color.y, color.z, 1.0f };

            material->bind();
            material->setVec3("color", color);
        }
    ));

    return properties;
}

nlohmann::json BoundingBoxParticleSpawnNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();

    j["restDensity"] = particleSpawn.restDensity;
    j["particleSpacing"] = particleSpawn.particleSpacing;
    j["color"] = { particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z };

    return j;
}

std::string BoundingBoxParticleSpawnNode::getTypeName() const {
    return "BoundingBoxFluidSpawnNode";
}

//
// Created by ruut on 6/11/26.
//

#include "ModelParticleSpawnNode.h"
#include "BoundingBoxNode.h"
#include <cuda_runtime_api.h>
#include <vector_functions.h>

ModelParticleSpawnNode::ModelParticleSpawnNode(const std::string& name, const std::shared_ptr<Model>& model)
    : ModelNode(name, model)
{
    this->name = name;
    this->boundary = false;

    init();
}

ModelParticleSpawnNode::ModelParticleSpawnNode(const nlohmann::json json) : ModelNode(json) {
    // Particle spawn
    particleSpawn.restDensity = json["restDensity"];
    particleSpawn.particleSpacing = json["particleSpacing"];
    particleSpawn.color = make_float4(
        json["color"][0].get<float>(),
        json["color"][1].get<float>(),
        json["color"][2].get<float>(),
        1.0f
    );

    init();
}

ModelParticleSpawnNode::~ModelParticleSpawnNode() {
    cudaFree(particleSpawn.d_shapeTriangles);
}

CudaParticleSpawn ModelParticleSpawnNode::getParticleSpawn() {
    // Get current world space triangles of model
    std::vector<Triangle> triangles = this->model->getWorldSpaceTriangles(transform);
    cudaMemcpy(particleSpawn.d_shapeTriangles, triangles.data(), particleSpawn.triangleCount * sizeof(Triangle), cudaMemcpyHostToDevice);

    particleSpawn.area = getWorldBoundingBox().toCudaAABB();
    return particleSpawn;
}

Properties ModelParticleSpawnNode::buildProperties() {
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

    properties.addSection("Debug");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Show Bounding Box",
        [this] () { return boundingBox->isVisible(); },
        [this] (bool on) { boundingBox->setVisible(on); }
    ));

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

            model->getMaterial(0)->bind();
            model->getMaterial(0)->setVec3("albedo", color);
        }
    ));

    return properties;
}

nlohmann::json ModelParticleSpawnNode::toJson() const {
    nlohmann::json j = ModelNode::toJson();

    j["restDensity"] = particleSpawn.restDensity;
    j["particleSpacing"] = particleSpawn.particleSpacing;
    j["color"] = { particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z };

    return j;
}

std::string ModelParticleSpawnNode::getTypeName() const {
    return "ModelParticleSpawnNode";
}

void ModelParticleSpawnNode::init() {
    particleSpawn.hasShape = true;

    this->model->clearMaterials();

    // Add unshaded material
    Material wireframe("Wireframe", Shader::createDefaultUnshaded());
    wireframe.setFillMode(FillMode::Wireframe);
    wireframe.setCullFaces(false);
    wireframe.setVec3("albedo", glm::vec3(particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z));

    this->model->addMaterial(wireframe);
    for (const auto m : this->model->getMeshes())
        m->setMaterialIndex(0);

    std::vector<Triangle> triangles = this->model->getWorldSpaceTriangles(glm::mat4(1.0f));
    particleSpawn.triangleCount = triangles.size();
    cudaMalloc(reinterpret_cast<void**>(&particleSpawn.d_shapeTriangles), particleSpawn.triangleCount * sizeof(Triangle));
}

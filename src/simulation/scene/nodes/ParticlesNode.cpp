//
// Created by ruut on 6/4/26.
//

#include "simulation/scene/nodes/ParticlesNode.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"
#include "rendering/Renderer.h"
#include "glm/gtc/type_ptr.hpp"
#include "rendering/RendererParticle.h"
#include "simulation/scene/Properties.h"
#include "simulation/scene/Scene.h"

ParticlesNode::ParticlesNode(FluidSimulation* simulation) : simulation(simulation), mesher(simulation) {
    // --- Models ---
    particleModel = std::make_shared<Model>("Particle", Mesh::createSphere("Particle Mesh", 4, 2));

    Material particleMaterial("Particle", Shader::createParticle());
    this->particleShader = particleMaterial.getShader();
    particleModel->addMaterial(particleMaterial);

    surfaceModel = mesher.getSurfaceModel();

    // ---------

    visible = false;
    sceneGraphVisible = false;
    savable = false;
    addTag("fluid");

    setupInstancing();
    updateRenderingSettings();
}

void ParticlesNode::update(float deltaTime) { }

void ParticlesNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    if (settings.mode == MARCHING_CUBES) {
        mesher.updateMesh(scene->getSimulationDomain()->getWorldBoundingBox());
        surfaceModel->draw(view, projection, glm::mat4(1.0f));
        return;
    }

    if (settings.mode == SCREEN_SPACE) {
        Framebuffer* sceneBuffer = Framebuffer::getCurrent();
        screenSpace.draw(sceneBuffer, view, projection);
        return;
    }

    particleModel->draw(view, projection, glm::mat4(1.0f));
}

Properties ParticlesNode::buildProperties() {
    return {};
}

Shader* ParticlesNode::getParticleShader() const {
    return particleShader;
}

unsigned int ParticlesNode::getParticleVBO() const {
    return particleVBO;
}

void ParticlesNode::updateRenderingSettings() const {
    particleShader->bind();

    particleShader->setFloat("size", settings.particleSize);
    particleShader->setInt("mode", settings.mode);

    particleShader->setVec3("minVelColor", settings.minVelColor);
    particleShader->setVec3("maxVelColor", settings.maxVelColor);
    particleShader->setFloat("minVel", settings.velocityMin);
    particleShader->setFloat("maxVel", settings.velocityMax);

    particleShader->setVec3("minPresColor", settings.minPresColor);
    particleShader->setVec3("maxPresColor", settings.maxPresColor);
    particleShader->setFloat("minPres", settings.pressureMin);
    particleShader->setFloat("maxPres", settings.pressureMax);

    particleShader->setFloat("maxSurfaceTension", settings.maxSurfaceTension);
}

void ParticlesNode::updateParticlesSize(int count) const {
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RendererParticle) * count, nullptr, GL_DYNAMIC_DRAW);

    particleModel->setInstanceCount(count);
    screenSpace.updateParticleSize(count);
}

FluidSurfaceMesher* ParticlesNode::getSurfaceMesher() {
    return &mesher;
}

FluidSurfaceScreenSpace* ParticlesNode::getSurfaceScreenSpace() {
    return &screenSpace;
}

void ParticlesNode::setupInstancing() {
    glGenBuffers(1, &particleVBO);

    for (auto* mesh : { particleModel->getMeshes()[0], screenSpace.getParticleMesh() }) {
        glBindVertexArray(mesh->getVAO());
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)0);
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);

        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, velocity));
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);

        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, properties));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);

        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, flags));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);

        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, color));
        glEnableVertexAttribArray(7);
        glVertexAttribDivisor(7, 1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticlesNode::updateSize(int width, int height) {
    screenSpace.resizeBuffers(width, height);
}

int ParticlesNode::getRenderPriority() const {
    return 10;
}

//
// Created by ruut on 3/9/26.
//

#define GLAD_GL_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL

#include "rendering/FluidRenderer.h"
#include <glad/gl.h>
#include "../simulation/FluidSimulation.h"
#include "utils/Logger.h"
#include "utils/SphereCreator.h"
#include <vector>

#include "ModelNode.h"
#include "Scene.h"

FluidRenderer::FluidRenderer(Scene& scene, FluidSimulation* simulation)
    : simulation(simulation)
{
    // Setup OpenGL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup framebuffer
    framebuffer = new Framebuffer(1, 1);
    framebuffer->addColorAttachment(GL_LINEAR);
    framebuffer->addDepthBuffer();

    // Setup particles
    setupParticles(scene);
    updateRenderingSettings();
}

FluidRenderer::~FluidRenderer() {
    delete framebuffer;
}

void FluidRenderer::renderNode(const SceneNode* node, const glm::mat4& view, const glm::mat4& projection) const {
    if (!node->isVisible())
        return;

    node->draw(view, projection);

    for (const auto& child : node->getChildren())
        renderNode(child, view, projection);
}

void FluidRenderer::render(const Scene& scene) const {
    // Render into framebuffer
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera transform
    auto camera = scene.getCamera();
    auto view = camera->getViewMatrix();
    auto projection = camera->getProjectionMatrix();

    for (const auto node : scene.getSceneNodes())
        renderNode(node, view, projection);
}

Shader* FluidRenderer::getParticleShader() const {
    return particleShader;
}

void FluidRenderer::updateParticleSize() const {
    particleShader->bind();
    particleShader->setFloat("size", settings.particleSize);
}

GLuint FluidRenderer::getTextureId() const {
    return framebuffer->getColorAttachment(0);
}

void FluidRenderer::setSize(int width, int height) const {
    auto size = framebuffer->getSize();
    if (size.x != width || size.y != height)
        framebuffer->resize(width, height);
}

GLuint FluidRenderer::getParticleVBO() const { return particleVBO; }

void FluidRenderer::setupParticles(Scene& scene) {
    // Create particle model
    auto particleModel = std::make_shared<Model>(
        "Particle",
        Mesh::createSphere("Particle Mesh", 4, 2)
    );

    Material particleMaterial("Particle", Shader::createParticle());
    this->particleShader = particleMaterial.getShader();
    particleModel->addMaterial(particleMaterial);

    auto particleModelNode = std::make_unique<ModelNode>("Particles", particleModel);
    this->particleModelNode = particleModelNode.get();

    particleModelNode->setVisible(false);
    particleModelNode->setSceneGraphVisible(false);
    particleModelNode->setBoundary(false);

    scene.addSceneNode(std::move(particleModelNode));

    // Setup instancing data
    glBindVertexArray(this->particleModelNode->getModel()->getMeshes()[0]->getVAO());

    glGenBuffers(1, &particleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, velocity));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, properties));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, flags));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FluidRenderer::updateParticleData() const {
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RendererParticle) * simulation->particleCount, nullptr, GL_DYNAMIC_DRAW);
    simulation->setRendererVBO(particleVBO);
    particleModelNode->getModel()->setInstanceCount(simulation->particleCount);
    particleModelNode->setVisible(settings.mode != MARCHING_CUBES);
}

void FluidRenderer::updateRenderingSettings() const {
    // Update particle shader
    particleShader->bind();

    particleShader->setInt("mode", settings.mode);
    particleShader->setFloat("size", settings.particleSize);

    particleShader->setVec4("particleColor", settings.solidColorParticles);

    particleShader->setVec4("minVelColor", settings.minVelColor);
    particleShader->setVec4("maxVelColor", settings.maxVelColor);
    particleShader->setFloat("minVel", settings.velocityMin);
    particleShader->setFloat("maxVel", settings.velocityMax);

    particleShader->setVec4("minPresColor", settings.minPresColor);
    particleShader->setVec4("maxPresColor", settings.maxPresColor);
    particleShader->setFloat("minPres", settings.pressureMin);
    particleShader->setFloat("maxPres", settings.pressureMax);

    particleShader->setFloat("maxSurfaceTension", settings.maxSurfaceTension);

    // Update particle visibility
    if (simulation->state != UNINITIALIZED) {
        particleModelNode->setVisible(settings.mode != MARCHING_CUBES);
    }
}
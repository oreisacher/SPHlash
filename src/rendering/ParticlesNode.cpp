//
// Created by ruut on 6/4/26.
//

#include "ParticlesNode.h"
#include "Renderer.h"
#include <iostream>
#include "glm/gtc/type_ptr.hpp"

ParticlesNode::ParticlesNode(FluidSimulation* simulation) : simulation(simulation), mesher(simulation) {
    // --- Models Setup ---
    particleModel = std::make_shared<Model>("Particle", Mesh::createSphere("Particle Mesh", 4, 2));

    Material particleMaterial("Particle", Shader::createParticle());
    this->particleShader = particleMaterial.getShader();
    particleModel->addMaterial(particleMaterial);

    billboardParticleModel = std::make_shared<Model>("Particle Billboard", Mesh::createQuad("Particle Billboard"));

    Material billboardParticleMaterial("Billboard Particle", Shader::createScreenSpaceParticle());
    this->billboardParticleShader = billboardParticleMaterial.getShader();
    billboardParticleModel->addMaterial(billboardParticleMaterial);

    surfaceModel = mesher.getSurfaceModel();

    // --- Frame buffers ---

    sceneDepthBuffer = std::make_unique<Framebuffer>(1, 1);
    sceneDepthBuffer->addColorAttachment(GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
    sceneDepthBuffer->addDepthBuffer();

    depthBuffer = std::make_unique<Framebuffer>(1, 1);
    depthBuffer->addColorAttachment(GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
    depthBuffer->addDepthBuffer();

    depthShader = Shader::createDepth();

    smoothBuffer1 = std::make_unique<Framebuffer>(1, 1);
    smoothBuffer1->addColorAttachment(GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
    smoothBuffer1->addDepthBuffer();

    smoothBuffer2 = std::make_unique<Framebuffer>(1, 1);
    smoothBuffer2->addColorAttachment(GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);

    blurShader = Shader::createBlur();

    normalBuffer = std::make_unique<Framebuffer>(1, 1);
    normalBuffer->addColorAttachment(GL_LINEAR, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    normalShader = Shader::createNormal();

    compositionBuffer = std::make_unique<Framebuffer>(1, 1);
    compositionBuffer->addColorAttachment(GL_LINEAR, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    compositionShader = Shader::createComposition();
    fullscreenShader = Shader::createFullscreen();

    glGenVertexArrays(1, &emptyVAO);

    // ---------

    visible = false;
    sceneGraphVisible = false;
    savable = false;

    setupInstancing();
    updateRenderingSettings();
}

void ParticlesNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    if (settings.mode == MARCHING_CUBES) {
        mesher.updateMesh();
        surfaceModel->draw(view, projection, glm::mat4(1.0f));
        return;
    }

    if (settings.mode == SCREEN_SPACE) {
        Framebuffer* sceneBuffer = Framebuffer::getCurrent();

        // --- Depth of Particles ---
        depthBuffer->bind();
        glDisable(GL_BLEND);
        glViewport(0, 0, depthBuffer->getSize().x, depthBuffer->getSize().y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        billboardParticleModel->draw(view, projection, glm::mat4(1.0f));

        // --- Horizontal Blur ---
        smoothBuffer1->bind();
        glDisable(GL_BLEND);
        glViewport(0, 0, depthBuffer->getSize().x, depthBuffer->getSize().y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        blurShader->bind();
        blurShader->setInt("depthTex", 0);
        blurShader->setVec2("direction", glm::vec2(1.0f, 0.0));
        blurShader->setVec2("texelSize", glm::vec2(1.0f / smoothBuffer1->getSize().x, 1.0 / smoothBuffer1->getSize().y));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthBuffer->getColorAttachment(0));

        glBindVertexArray(emptyVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // --- Vertical Blur ---
        smoothBuffer2->bind();
        glDisable(GL_BLEND);
        glViewport(0, 0, depthBuffer->getSize().x, depthBuffer->getSize().y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        blurShader->bind();
        blurShader->setVec2("direction", glm::vec2(0.0f, 1.0));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smoothBuffer1->getColorAttachment(0));

        glBindVertexArray(emptyVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // --- Normal Reconstruction ---
        normalBuffer->bind();
        glDisable(GL_BLEND);
        glViewport(0, 0, depthBuffer->getSize().x, depthBuffer->getSize().y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        normalShader->bind();
        normalShader->setInt("smoothDepth", 0);
        normalShader->setVec2("texelSize", glm::vec2(1.0f / smoothBuffer2->getSize().x, 1.0 / smoothBuffer2->getSize().y));
        normalShader->setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smoothBuffer2->getColorAttachment(0));

        glBindVertexArray(emptyVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // --- Composition ---
        compositionBuffer->bind();
        glViewport(0, 0, smoothBuffer2->getSize().x, smoothBuffer2->getSize().y);

        compositionShader->bind();
        compositionShader->setInt("normalTex", 0);
        compositionShader->setInt("depthTex", 1);
        compositionShader->setInt("sceneTex", 2);
        compositionShader->setInt("sceneDepthTex", 3);
        compositionShader->setVec3("fluidColor", glm::vec3(0.2, 0.2, 1.0));
        compositionShader->setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, normalBuffer->getColorAttachment(0));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, smoothBuffer2->getColorAttachment(0));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sceneBuffer->getColorAttachment(0));
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, sceneBuffer->getDepthTexture());

        glBindVertexArray(emptyVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // --- SceneBuffer ---
        sceneBuffer->bind();
        glViewport(0, 0, smoothBuffer2->getSize().x, smoothBuffer2->getSize().y);

        fullscreenShader->bind();
        fullscreenShader->setInt("tex", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, compositionBuffer->getColorAttachment(0));

        glBindVertexArray(emptyVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

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

    particleShader->setVec4("minVelColor", settings.minVelColor);
    particleShader->setVec4("maxVelColor", settings.maxVelColor);
    particleShader->setFloat("minVel", settings.velocityMin);
    particleShader->setFloat("maxVel", settings.velocityMax);

    particleShader->setVec4("minPresColor", settings.minPresColor);
    particleShader->setVec4("maxPresColor", settings.maxPresColor);
    particleShader->setFloat("minPres", settings.pressureMin);
    particleShader->setFloat("maxPres", settings.pressureMax);

    particleShader->setFloat("maxSurfaceTension", settings.maxSurfaceTension);

    billboardParticleShader->bind();
    billboardParticleShader->setFloat("billboardSize", settings.billboardSize);
}

void ParticlesNode::updateParticlesSize(int count) const {
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RendererParticle) * count, nullptr, GL_DYNAMIC_DRAW);

    particleModel->setInstanceCount(count);
    billboardParticleModel->setInstanceCount(count);
}

FluidSurfaceMesher* ParticlesNode::getSurfaceMesher() {
    return &mesher;
}

void ParticlesNode::setupInstancing() {
    glGenBuffers(1, &particleVBO);

    for (auto* mesh : { particleModel->getMeshes()[0], billboardParticleModel->getMeshes()[0] }) {
        glBindVertexArray(mesh->getVAO());
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)0);
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);

        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, velocity));
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);

        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, properties));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);

        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, flags));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);

        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, color));
        glEnableVertexAttribArray(7);
        glVertexAttribDivisor(7, 1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticlesNode::updateSize(int width, int height) {
    depthBuffer->resize(width, height);
    smoothBuffer1->resize(width, height);
    smoothBuffer2->resize(width, height);
    normalBuffer->resize(width, height);
    compositionBuffer->resize(width, height);
}

int ParticlesNode::getRenderPriority() const {
    return 10;
}

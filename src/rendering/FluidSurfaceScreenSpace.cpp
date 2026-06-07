//
// Created by ruut on 6/7/26.
//

#include "FluidSurfaceScreenSpace.h"
#include "Framebuffer.h"
#include "glm/gtc/type_ptr.inl"

FluidSurfaceScreenSpace::FluidSurfaceScreenSpace() {
    glGenVertexArrays(1, &screenVAO);

    // --- Model ---
    quadParticleModel = std::make_shared<Model>("Particle Quad", Mesh::createQuad("Particle Quad"));

    Material particleMaterial("Particle Quad", Shader::createScreenSpaceParticle());
    this->particleShader = particleMaterial.getShader();
    quadParticleModel->addMaterial(particleMaterial);

    // --- Buffers ---
    particlesDepthBuffer = std::make_unique<Framebuffer>(1, 1);
    particlesDepthBuffer->addColorAttachment(GL_LINEAR, GL_R32F, GL_RED, GL_FLOAT);
    particlesDepthBuffer->addDepthTexture();

    blurBuffer1 = std::make_unique<Framebuffer>(1, 1);
    blurBuffer1->addColorAttachment(GL_LINEAR, GL_R32F, GL_RED, GL_FLOAT);

    blurBuffer2 = std::make_unique<Framebuffer>(1, 1);
    blurBuffer2->addColorAttachment(GL_LINEAR, GL_R32F, GL_RED, GL_FLOAT);

    normalBuffer = std::make_unique<Framebuffer>(1, 1);
    normalBuffer->addColorAttachment(GL_LINEAR, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    compositionBuffer = std::make_unique<Framebuffer>(1, 1);
    compositionBuffer->addColorAttachment(GL_LINEAR, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    // --- Shaders ---
    blurShader = Shader::createBlur();
    normalShader = Shader::createNormal();
    compositionShader = Shader::createComposition();
    fullscreenShader = Shader::createFullscreen();

    updateRenderingSettings();
}

void FluidSurfaceScreenSpace::draw(Framebuffer* dest, const glm::mat4& view, const glm::mat4& projection) const {
    glDisable(GL_BLEND);
    glm::vec2 texelSize = { 1.0f / dest->getSize().x, 1.0f / dest->getSize().y };

    glBindFramebuffer(GL_READ_FRAMEBUFFER, dest->getID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, particlesDepthBuffer->getID());
    glBlitFramebuffer(
        0, 0, dest->getSize().x, dest->getSize().y,
        0, 0, dest->getSize().x, dest->getSize().y,
        GL_DEPTH_BUFFER_BIT,
        GL_NEAREST
    );

    // --- Depth of Particles ---
    particlesDepthBuffer->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    quadParticleModel->draw(view, projection, glm::mat4(1.0f));

    // --- Horizontal Blur ---
    blurBuffer1->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    blurShader->bind();
    blurShader->setInt("depthTex", 0);
    blurShader->setVec2("direction", glm::vec2(1.0f, 0.0));
    blurShader->setVec2("texelSize", texelSize);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particlesDepthBuffer->getColorAttachment(0));

    drawFullscreen();

    // --- Vertical Blur ---
    blurBuffer2->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    blurShader->setVec2("direction", glm::vec2(0.0f, 1.0));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffer1->getColorAttachment(0));

    drawFullscreen();

    // --- Normal Reconstruction ---
    normalBuffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    normalShader->bind();
    normalShader->setInt("smoothDepth", 0);
    normalShader->setVec2("texelSize", texelSize);
    normalShader->setMat4("projection", projection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffer2->getColorAttachment(0));

    drawFullscreen();

    // --- Composition ---
    compositionBuffer->bind();

    compositionShader->bind();
    compositionShader->setInt("sceneTex", 0);
    compositionShader->setInt("particlesDepthTex", 1);
    compositionShader->setInt("normalTex", 2);
    compositionShader->setMat4("view", view);
    compositionShader->setMat4("projection", projection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dest->getColorAttachment(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurBuffer2->getColorAttachment(0));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalBuffer->getColorAttachment(0));

    drawFullscreen();

    // --- SceneBuffer ---
    dest->bind();

    fullscreenShader->bind();
    fullscreenShader->setInt("tex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, compositionBuffer->getColorAttachment(0));

    drawFullscreen();
}

void FluidSurfaceScreenSpace::resizeBuffers(int width, int height) const {
    particlesDepthBuffer->resize(width, height);
    blurBuffer1->resize(width, height);
    blurBuffer2->resize(width, height);
    normalBuffer->resize(width, height);
    compositionBuffer->resize(width, height);
}

void FluidSurfaceScreenSpace::updateParticleSize(int count) const {
    quadParticleModel->setInstanceCount(count);
}

Mesh* FluidSurfaceScreenSpace::getParticleMesh() const {
    return quadParticleModel->getMeshes()[0];
}

void FluidSurfaceScreenSpace::updateRenderingSettings() const {
    particleShader->bind();
    particleShader->setFloat("size", settings.particleSize);

    blurShader->bind();
    blurShader->setInt("kernel", settings.blurSize);
    blurShader->setFloat("depthThreshold", settings.depthThreshold);

    compositionShader->bind();
    compositionShader->setVec3("fluidColor", settings.fluidColor);
    compositionShader->setFloat("transparency", settings.transparency);
    compositionShader->setFloat("ambientStrength", settings.ambientStrength);
    compositionShader->setFloat("specularStrength", settings.specularStrength);
    compositionShader->setFloat("fresnelStrength", settings.fresnelStrength);
}

void FluidSurfaceScreenSpace::drawFullscreen() const {
    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

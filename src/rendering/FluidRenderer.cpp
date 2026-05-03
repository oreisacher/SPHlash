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

FluidRenderer::FluidRenderer() {
    simulation = FluidSimulation::getInstance();

    // Setup OpenGL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Setup framebuffer
    framebuffer = new Framebuffer(1, 1);
    framebuffer->addColorAttachment();
    framebuffer->addDepthBuffer();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Setup sphere
    setupSphereModel();

    // Setup particle
    setupParticles();

    // Setup shaders
    setupShaders();
}

FluidRenderer::~FluidRenderer() {
    delete framebuffer;
}

FluidRenderer* FluidRenderer::getInstance() {
    static FluidRenderer instance;
    return &instance;
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

    // Draw particles if particles are there and no marching cubes is active
    if (simulation->particleCount != 0 && settings.mode != MARCHING_CUBES) {
        particleShader->bind();
        particleShader->setMat4("projection", camera->getProjectionMatrix());
        particleShader->setMat4("view", camera->getViewMatrix());

        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0, simulation->particleCount);
    }

    for (const auto model : scene.getModels()) {
        if (!model->isVisible())
            continue;

        model->draw(view, projection);
    }
}

Shader* FluidRenderer::getParticleShader() const {
    return particleShader;
}

Shader* FluidRenderer::getBoundaryShader() const {
    return boxShader;
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

void FluidRenderer::setupShaders() {
    // Particle Shader
    particleShader = new Shader({
        { "assets/shaders/default.vert", Shader::VERTEX },
        { "assets/shaders/default.frag", Shader::FRAGMENT },
    });

    particleShader->compile();
    particleShader->bind();

    // Set default settings
    particleShader->setInt("mode", settings.mode);
    particleShader->setFloat("size", settings.particleSize);
    particleShader->setVec3("particleColor", settings.solidColorParticles);
}

void FluidRenderer::setupSphereModel() {
    // Generate mesh data for sphere
    std::vector<SphereVertex> vertices;
    std::vector<unsigned int> indices;
    createSphere(vertices, indices, 4, 2);
    sphereIndexCount = indices.size();

    // Vertex buffer
    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SphereVertex), vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glGenBuffers(1, &sphereEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)offsetof(SphereVertex, normal));
    glEnableVertexAttribArray(1);
}

void FluidRenderer::setupParticles() {
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

    // glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, flags));
    // glEnableVertexAttribArray(5);
    // glVertexAttribDivisor(5, 1)
    //
    // glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, debugging));
    // glEnableVertexAttribArray(6);
    // glVertexAttribDivisor(6, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // simulation->setRendererVBO(particleVBO);
}

void FluidRenderer::updateParticleData() const {
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RendererParticle) * simulation->particleCount, nullptr, GL_DYNAMIC_DRAW);
    simulation->setRendererVBO(particleVBO);
}

void FluidRenderer::updateRenderingSettings() const {
    particleShader->bind();

    particleShader->setInt("mode", settings.mode);
    particleShader->setFloat("size", settings.particleSize);

    particleShader->setVec3("particleColor", settings.solidColorParticles);

    particleShader->setVec3("minVelColor", settings.minVelColor);
    particleShader->setVec3("maxVelColor", settings.maxVelColor);
    particleShader->setFloat("minVel", settings.velocityMin);
    particleShader->setFloat("maxVel", settings.velocityMax);

    particleShader->setVec3("minPresColor", settings.minPresColor);
    particleShader->setVec3("maxPresColor", settings.maxPresColor);
    particleShader->setFloat("minPres", settings.pressureMin);
    particleShader->setFloat("maxPres", settings.pressureMax);
}

//
// Created by ruut on 3/9/26.
//

#define GLAD_GL_IMPLEMENTATION

#include "rendering/FluidRenderer.h"
#include <glad/gl.h>
#include "../simulation/FluidSimulation.h"
#include "GLFW/glfw3.h"
#include "utils/Logger.h"
#include "utils/SphereCreator.h"

FluidRenderer::FluidRenderer(FluidSimulationGPU* sim) : simulation(sim) {
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

    // Setup Spawn Area box
    setupSpawnAreaModel();

    // Setup shaders
    setupShaders();
}

FluidRenderer::~FluidRenderer() {
    delete framebuffer;
    delete boundaryShader;
}

void FluidRenderer::render() const {
    // Update shader
    particleShader->bind();
    particleShader->setMat4("projection", camera->getProjectionMatrix());
    particleShader->setMat4("view", camera->getViewMatrix());

    // Render into framebuffer
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw particles
    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0, simulation->particleCount);

    // Draw bounding box
    boundaryShader->bind();
    boundaryShader->setMat4("projection", camera->getProjectionMatrix());
    boundaryShader->setMat4("view", camera->getViewMatrix());
    glBindVertexArray(boxVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
}

void FluidRenderer::setCamera(Camera* cam) {
    camera = cam;
}

Shader* FluidRenderer::getParticleShader() const {
    return particleShader;
}

Shader* FluidRenderer::getBoundaryShader() const {
    return boundaryShader;
}

void FluidRenderer::updateParticleSize() const {
    particleShader->bind();
    particleShader->setFloat("size", settings.particleSize);
}

void FluidRenderer::rebuildSpawnAreaBox() const {
    //      7 -------- 6
    //     /|         /|
    //    3 -------- 2 |
    //    | |        | |
    //    | 4 -------| 5
    //    |/         |/
    //    0 -------- 1

    float vertices[] = {
        simulation->initSettings.xBoundary.x, simulation->initSettings.yBoundary.x, simulation->initSettings.zBoundary.x, // 0
        simulation->initSettings.xBoundary.y, simulation->initSettings.yBoundary.x, simulation->initSettings.zBoundary.x, // 1
        simulation->initSettings.xBoundary.y, simulation->initSettings.yBoundary.y, simulation->initSettings.zBoundary.x, // 2
        simulation->initSettings.xBoundary.x, simulation->initSettings.yBoundary.y, simulation->initSettings.zBoundary.x, // 3
        simulation->initSettings.xBoundary.x, simulation->initSettings.yBoundary.x, simulation->initSettings.zBoundary.y, // 4
        simulation->initSettings.xBoundary.y, simulation->initSettings.yBoundary.x, simulation->initSettings.zBoundary.y, // 5
        simulation->initSettings.xBoundary.y, simulation->initSettings.yBoundary.y, simulation->initSettings.zBoundary.y, // 6
        simulation->initSettings.xBoundary.x, simulation->initSettings.yBoundary.y, simulation->initSettings.zBoundary.y, // 7
    };

    unsigned int indices[] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
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
    particleShader->setFloat("size", settings.particleSize);

    // Boundary Box Shader
    boundaryShader = new Shader({
        { "assets/shaders/box.vert", Shader::VERTEX },
        { "assets/shaders/box.frag", Shader::FRAGMENT },
    });

    boundaryShader->compile();
}

void FluidRenderer::setupSphereModel() {
    // Generate mesh data for sphere
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    createSphere(vertices, indices, 4, 2);
    sphereIndexCount = indices.size();

    // Vertex buffer
    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glGenBuffers(1, &sphereEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
}

void FluidRenderer::setupParticles() {
    glGenBuffers(1, &particleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(float4) * simulation->particleCount, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float4), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, velocity));
    // glEnableVertexAttribArray(3);
    // glVertexAttribDivisor(3, 1);
    //
    // glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, properties));
    // glEnableVertexAttribArray(4);
    // glVertexAttribDivisor(4, 1);
    //
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

void FluidRenderer::setupSpawnAreaModel() {
    glGenVertexArrays(1, &boxVAO);
    glBindVertexArray(boxVAO);

    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &boxEBO);

    rebuildSpawnAreaBox();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void FluidRenderer::updateParticleData() const {
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ParticleGPU) * simulation->particleCount, simulation->particles.data(), GL_DYNAMIC_DRAW);
    simulation->setRendererVBO(particleVBO);
}

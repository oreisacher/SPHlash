//
// Created by ruut on 3/9/26.
//

#define GLAD_GL_IMPLEMENTATION

#include "FluidRenderer.h"
#include <glad/gl.h>
#include "../simulation/FluidSimulation.h"
#include "GLFW/glfw3.h"
#include "utils/Logger.h"

FluidRenderer::FluidRenderer(const FluidSimulationGPU* sim) {
    if (!initialized) {
        LOG_ERROR("Renderer is not initialized. Call `Renderer::Init()` before use.")
        return;
    }

    this->simulation = sim;

    // Setup framebuffer
    framebuffer = new Framebuffer(1, 1);
    framebuffer->addColorAttachment();

    // Setup quad VAO for particle drawing
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ParticleGPU) * simulation->particleCount, simulation->particles.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, velocity));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, properties));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
}

FluidRenderer::~FluidRenderer() {
    delete framebuffer;
}

void FluidRenderer::Init() {
    if (!gladLoadGL(glfwGetProcAddress)) {
        LOG_FATAL("Failed to initialize GLAD");
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    initialized = true;
}

void FluidRenderer::render() const {
    // Update particles data
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle) * simulation->particleCount, simulation->particles.data());

    // Render into framebuffer
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, simulation->particleCount);
}

GLuint FluidRenderer::getTextureId() const {
    return framebuffer->getColorAttachment(0);
}

void FluidRenderer::setSize(int width, int height) const {
    auto size = framebuffer->getSize();
    if (size.x != width || size.y != height)
        framebuffer->resize(width, height);
}

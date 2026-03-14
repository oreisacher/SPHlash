//
// Created by ruut on 3/9/26.
//

#include "FluidRenderer.h"

#include "../FluidSimulation.h"

FluidRenderer::FluidRenderer(const FluidSimulation* sim) {
    this->simulation = sim;

    // Setup framebuffer
    framebuffer = new Framebuffer(1, 1);
    framebuffer->addColorAttachment();

    // Setup quad VAO for particle drawing
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * simulation->getParticleCount(), simulation->getData(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, velocity));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, pressure));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
}

FluidRenderer::~FluidRenderer() {
    delete framebuffer;
}

void FluidRenderer::render() const {
    // Update particles data
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle) * simulation->getParticleCount(), simulation->getData());

    // Render into framebuffer
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, simulation->getParticleCount());
}

GLuint FluidRenderer::getTextureId() const {
    return framebuffer->getColorAttachment(0);
}

void FluidRenderer::setSize(int width, int height) const {
    auto size = framebuffer->getSize();
    if (size.x != width || size.y != height)
        framebuffer->resize(width, height);
}

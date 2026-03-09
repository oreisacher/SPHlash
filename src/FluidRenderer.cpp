//
// Created by ruut on 3/9/26.
//

#include "FluidRenderer.h"

#include "FluidSimulation.h"

FluidRenderer::FluidRenderer(const FluidSimulation* sim) {
    this->simulation = sim;

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
}

void FluidRenderer::update() {
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle) * simulation->getParticleCount(), simulation->getData());
}

void FluidRenderer::render() {
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, simulation->getParticleCount());
}

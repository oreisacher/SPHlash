//
// Created by ruut on 3/16/26.
//

#include "FluidSimulationGPU.h"

#include <iostream>
#include <ostream>
#include <random>
#include "glad/gl.h"

FluidSimulationGPU::FluidSimulationGPU(int particleCount, float xMin, float yMin, float xMax, float yMax) {
    // Setup particles
    particles.resize(particleCount);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(xMin, xMax);
    std::uniform_real_distribution<float> distY(yMin, yMax);

    float mass = 1000.0f * (((xMax - xMin) * (yMax - yMin)) / particleCount);

    for (int i = 0 ; i < particleCount ; i++) {
        particles[i].position = { distX(gen), distY(gen), 0.0, 0.0 };
        particles[i].velocity = { 0.0, 0.0, 0.0, 0.0 };
        particles[i].properties = { mass, 0.0, 0.0, 0.0 };
    }

    initParticlesSSBO();

    // Setup hash data
    hashData.resize(particleCount);
    initHashDataSSBO();

    // Setup shaders
    hashParticlesShader = new Shader({{ "assets/shaders/hashParticles.compute", Shader::Type::COMPUTE}});
    hashParticlesShader->compile();
    hashParticlesShader->bind();
    hashParticlesShader->setFloat("cellSize", 0.016); // TODO - Replace with real value
    hashParticlesShader->setInt("numParticles", particleCount);
}

FluidSimulationGPU::~FluidSimulationGPU() {
    delete hashParticlesShader;
}

void FluidSimulationGPU::update(float deltaTime) {
    // Hash particle position
    glDispatchCompute(particles.size() / 16, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Sort by hash to group particles -> Radix sort

    // Build lookup table

    // Actual Simulation
}

void FluidSimulationGPU::initParticlesSSBO() {
    glGenBuffers(1, &particleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        particles.size() * sizeof(ParticleGPU),
        particles.data(),
        GL_DYNAMIC_DRAW
    );

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
}

void FluidSimulationGPU::initHashDataSSBO() {
    glGenBuffers(1, &hashDataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, hashDataSSBO);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        hashData.size() * sizeof(HashEntry),
        hashData.data(),
        GL_DYNAMIC_DRAW
    );

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, hashDataSSBO);
}

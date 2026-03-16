//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include "glm/glm.hpp"
#include "rendering/Shader.h"

struct ParticleGPU {
    glm::vec4 position;     // vec4 to avoid padding issues
    glm::vec4 velocity;
    glm::vec4 properties;   // x: mass, y: density, z: pressure, w: unused
};

struct HashEntry {
    uint32_t hash;
    uint32_t index;
};

class FluidSimulationGPU {
public:
    FluidSimulationGPU(int particleCount, float xMin, float yMin, float xMax, float yMax);
    ~FluidSimulationGPU();

    void update(float deltaTime);

private:
    std::vector<ParticleGPU> particles;
    std::vector<HashEntry> hashData;

    GLuint particleSSBO, hashDataSSBO;

    Shader* hashParticlesShader;

    void initParticlesSSBO();
    void initHashDataSSBO();
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
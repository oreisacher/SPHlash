//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
#define FLUIDSIMSPH_FLUIDSIMULATIONGPU_H

#include "glm/glm.hpp"
#include "rendering/Shader.h"
#include <cuda_runtime.h>

struct ParticleGPU {
    float4 position;     // vec4 to avoid padding issues
    float4 velocity;
    float4 properties;   // x: mass, y: density, z: pressure, w: unused
};

struct HashEntry {
    uint32_t hash;
    uint32_t index;
};

struct SimulationSettings {
    float h = 20.0f;
    float p_0 = 1000.0f;
    float k = 0.7f;
    float mu = 0.3f;
    float tension = 0.001f;

    bool enableGravity = false;
    float gravity = -9.878;
};

class FluidSimulationGPU {
public:
    FluidSimulationGPU(int particleCount, float xMin, float yMin, float xMax, float yMax);
    ~FluidSimulationGPU();

    void update(float deltaTime);

    void reset();

    void updateBoundaries(float width, float height);

    int particleCount;
    std::vector<ParticleGPU> particles;
    SimulationSettings settings;

private:
    std::vector<uint32_t> hashes;
    std::vector<uint32_t> hashIndices;
    std::vector<int32_t> cellStart;
    std::vector<int32_t> cellEnd;

    ParticleGPU* d_particles;
    // HashEntry* d_hashData;
    uint32_t* d_hashes;
    uint32_t* d_hashIndices;
    int32_t* d_cellStart;
    int32_t* d_cellEnd;


    GLuint particleSSBO, hashDataSSBO, histogramDataSSBO, offsetDataSSBO;

    Shader* hashParticlesShader, *histogramShader, *offsetShader;

    // Boundaries
    float width = 1, height = 1;

    void initParticlesSSBO();
    void initHashDataSSBO();
    void initHistogramDataSSBO();
    void initOffsetDataSSBO();
};

#endif //FLUIDSIMSPH_FLUIDSIMULATIONGPU_H
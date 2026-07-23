//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_PARTICLESHOST_H
#define SPHLASH_PARTICLESHOST_H

#include <vector>
#include <vector_types.h>

struct ParticlesGPU;

/*
    Represents all particles of simulation as SoA. Contains actual data. Used during initialization.
*/
struct ParticlesHost {
    std::vector<float4> position;
    std::vector<float4> startPosition;
    std::vector<float4> velocity;
    std::vector<float4> properties;
    std::vector<float4> flags;
    std::vector<float4> color;
    std::vector<float4> pressureForce;
    std::vector<float4> viscosityForce;
    std::vector<float4> surfaceTensionForce;

    void resize(size_t count);
    void append(ParticlesHost& particles);
    void copyToDevice(const ParticlesGPU& device, size_t count) const;
};

#endif //SPHLASH_PARTICLESHOST_H

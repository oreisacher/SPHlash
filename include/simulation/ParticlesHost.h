//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_PARTICLESHOST_H
#define SPHLASH_PARTICLESHOST_H

#include <vector>
#include "glm/glm.hpp"

struct ParticlesGPU;

/*
    Represents all particles of simulation as SoA. Contains actual data. Used during initialization.
*/
struct ParticlesHost {
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> startPosition;

    std::vector<glm::vec3> velocity;
    std::vector<glm::vec3> pressureForce;
    std::vector<glm::vec3> viscosityForce;
    std::vector<glm::vec3> surfaceTensionForce;

    std::vector<glm::vec4> properties;
    std::vector<glm::ivec4> flags;

    std::vector<glm::vec3> color;

    void resize(size_t count);
    void append(ParticlesHost& particles);
    void copyToDevice(const ParticlesGPU& device, size_t count) const;
};

#endif //SPHLASH_PARTICLESHOST_H

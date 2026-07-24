//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_PARTICLESGPU_H
#define SPHLASH_PARTICLESGPU_H

#include "glm/glm.hpp"

struct ParticlesHost;
struct ParticleHost;

/*
    Represents all particles of simulation as SoA. Contains device pointers. Used during simulation.
*/
struct ParticlesGPU {
    glm::vec3* d_position              = nullptr;
    glm::vec3* d_startPosition         = nullptr;

    glm::vec3* d_velocity              = nullptr;
    glm::vec3* d_pressureForce         = nullptr;
    glm::vec3* d_viscosityForce        = nullptr;
    glm::vec3* d_surfaceTensionForce   = nullptr;

    glm::vec4* d_properties            = nullptr;  //  x: mass,     y: density,     z: pressure,    w: id
    glm::vec4* d_flags                = nullptr;  //  x: surface,  y: undefined,   z: undefined,   w: undefined

    glm::vec3 * d_color                = nullptr;

    void allocate(size_t count);
    void free();
    void copyToHost(ParticlesHost& host, size_t count) const;
    void copyToHost(ParticleHost& host, int id) const;
};

#endif //SPHLASH_PARTICLESGPU_H

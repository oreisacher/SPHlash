//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_PARTICLEHOST_H
#define SPHLASH_PARTICLEHOST_H

#include "glm/glm.hpp"

/*
    Represents a single particle inside simulation. Can be queried via getParticle(id). Host only
*/
struct ParticleHost {
    glm::vec3 position;
    glm::vec3 startPosition;

    glm::vec3 velocity;
    glm::vec3 pressureForce;
    glm::vec3 viscosityForce;
    glm::vec3 surfaceTensionForce;

    glm::vec4 properties;       //  x: mass,     y: density,     z: pressure,    w: id
    glm::vec4 flags;            //  x: surface,  y: undefined,   z: undefined,   w: undefined

    glm::vec3 color;
};

#endif //SPHLASH_PARTICLEHOST_H

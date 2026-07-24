//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_IPARTICLESPAWN_H
#define SPHLASH_IPARTICLESPAWN_H

struct Triangle;

struct ParticleSpawn {
    float restDensity       = 1000.0f;
    float particleSpacing   = 0.025f;
    glm::vec3 color         = { 1.0f, 0.0f, 0.0f };

    AABBView area = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    };

    bool hasShape               = false;
    int triangleCount           = 0;
    Triangle* d_shapeTriangles  = nullptr;
};

class IParticleSpawn {
public:
    virtual ~IParticleSpawn() = default;
    virtual ParticleSpawn getParticleSpawn() = 0;
};

#endif //SPHLASH_IPARTICLESPAWN_H

//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_IPARTICLESPAWN_H
#define SPHLASH_IPARTICLESPAWN_H

struct Triangle;

struct CudaParticleSpawn {
    float restDensity       = 1000.0f;
    float particleSpacing   = 0.025f;
    float4 color            = { 1.0f, 0.0f, 0.0f, 1.0f };

    CudaAABB area = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    };

    bool hasShape               = false;
    Triangle* d_shapeTriangles  = nullptr;
};

class IParticleSpawn {
public:
    virtual ~IParticleSpawn() = default;
    virtual CudaParticleSpawn getParticleSpawn() = 0;
};

#endif //SPHLASH_IPARTICLESPAWN_H

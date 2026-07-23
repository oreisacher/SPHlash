//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_PARTICLESGPU_H
#define SPHLASH_PARTICLESGPU_H

#include <vector_types.h>
#include <cstddef>

struct ParticlesHost;
struct ParticleHost;

/*
    Represents all particles of simulation as SoA. Contains device pointers. Used during simulation.
*/
struct ParticlesGPU {
    float4* d_position              = nullptr;
    float4* d_startPosition         = nullptr;
    float4* d_velocity              = nullptr;
    float4* d_properties            = nullptr;  //  x: mass,     y: density,     z: pressure,    w: id
    float4* d_flags                 = nullptr;  //  x: surface,  y: undefined,   z: undefined,   w: undefined
    float4* d_color                 = nullptr;
    float4* d_pressureForce         = nullptr;
    float4* d_viscosityForce        = nullptr;
    float4* d_surfaceTensionForce   = nullptr;

    void allocate(size_t count);
    void free();
    void copyToHost(ParticlesHost& host, size_t count) const;
    void copyToHost(ParticleHost& host, int id) const;
};

#endif //SPHLASH_PARTICLESGPU_H

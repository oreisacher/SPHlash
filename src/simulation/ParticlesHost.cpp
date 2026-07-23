//
// Created by ruut on 7/23/26.
//

#include "simulation/ParticlesHost.h"
#include "simulation/ParticlesGPU.h"
#include <cuda_runtime_api.h>

void ParticlesHost::resize(size_t count) {
    position.resize(count);
    startPosition.resize(count);
    velocity.resize(count);
    properties.resize(count);
    flags.resize(count);
    color.resize(count);
    pressureForce.resize(count);
    viscosityForce.resize(count);
    surfaceTensionForce.resize(count);
}

void ParticlesHost::append(ParticlesHost& particles) {
    position.insert(position.end(), particles.position.begin(), particles.position.end());
    startPosition.insert(startPosition.end(), particles.startPosition.begin(), particles.startPosition.end());
    velocity.insert(velocity.end(), particles.velocity.begin(), particles.velocity.end());
    properties.insert(properties.end(), particles.properties.begin(), particles.properties.end());
    flags.insert(flags.end(), particles.flags.begin(), particles.flags.end());
    color.insert(color.end(), particles.color.begin(), particles.color.end());
    pressureForce.insert(pressureForce.end(), particles.pressureForce.begin(), particles.pressureForce.end());
    viscosityForce.insert(viscosityForce.end(), particles.viscosityForce.begin(), particles.viscosityForce.end());
    surfaceTensionForce.insert(surfaceTensionForce.end(), particles.surfaceTensionForce.begin(), particles.surfaceTensionForce.end());
}

void ParticlesHost::copyToDevice(const ParticlesGPU& device, const size_t count) const {
    // Copy data from host to device
    cudaMemcpy(device.d_position,               position.data(),               count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_startPosition,          startPosition.data(),          count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_velocity,               velocity.data(),               count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_properties,             properties.data(),             count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_flags,                  flags.data(),                  count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_color,                  color.data(),                  count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_pressureForce,          pressureForce.data(),          count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_viscosityForce,         viscosityForce.data(),         count * sizeof(float4), cudaMemcpyHostToDevice);
    cudaMemcpy(device.d_surfaceTensionForce,    surfaceTensionForce.data(),    count * sizeof(float4), cudaMemcpyHostToDevice);
}
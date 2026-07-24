//
// Created by ruut on 7/23/26.
//

#include "simulation/ParticlesGPU.h"
#include "simulation/ParticleHost.h"
#include "simulation/ParticlesHost.h"
#include <cuda_runtime_api.h>


void ParticlesGPU::allocate(const size_t count) {
    cudaMalloc(reinterpret_cast<void**>(&d_position),               count * sizeof(glm::vec3));
    cudaMalloc(reinterpret_cast<void**>(&d_startPosition),          count * sizeof(glm::vec3));
    cudaMalloc(reinterpret_cast<void**>(&d_velocity),               count * sizeof(glm::vec3));
    cudaMalloc(reinterpret_cast<void**>(&d_pressureForce),          count * sizeof(glm::vec3));
    cudaMalloc(reinterpret_cast<void**>(&d_viscosityForce),         count * sizeof(glm::vec3));
    cudaMalloc(reinterpret_cast<void**>(&d_surfaceTensionForce),    count * sizeof(glm::vec3));
    cudaMalloc(reinterpret_cast<void**>(&d_properties),             count * sizeof(glm::vec4));
    cudaMalloc(reinterpret_cast<void**>(&d_flags),                  count * sizeof(glm::vec4));
    cudaMalloc(reinterpret_cast<void**>(&d_color),                  count * sizeof(glm::vec3));
}

void ParticlesGPU::free() {
    cudaFree(d_position           ); d_position             = nullptr;
    cudaFree(d_startPosition      ); d_startPosition        = nullptr;
    cudaFree(d_velocity           ); d_velocity             = nullptr;
    cudaFree(d_properties         ); d_properties           = nullptr;
    cudaFree(d_flags              ); d_flags                = nullptr;
    cudaFree(d_color              ); d_color                = nullptr;
    cudaFree(d_pressureForce      ); d_pressureForce        = nullptr;
    cudaFree(d_viscosityForce     ); d_viscosityForce       = nullptr;
    cudaFree(d_surfaceTensionForce); d_surfaceTensionForce  = nullptr;
}

void ParticlesGPU::copyToHost(ParticlesHost& host, const size_t count) const {
    host.resize(count);

    // Copy data from device to host
    cudaMemcpy(host.position.data(),            d_position,             count * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.startPosition.data(),       d_startPosition,        count * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.velocity.data(),            d_velocity,             count * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.pressureForce.data(),       d_pressureForce,        count * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.viscosityForce.data(),      d_viscosityForce,       count * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.surfaceTensionForce.data(), d_surfaceTensionForce,  count * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.properties.data(),          d_properties,           count * sizeof(glm::vec4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.flags.data(),               d_flags,                count * sizeof(glm::vec4), cudaMemcpyDeviceToHost);
    cudaMemcpy(host.color.data(),               d_color,                count * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
}

void ParticlesGPU::copyToHost(ParticleHost& host, int id) const {
    cudaMemcpy(&host.position,              d_position + id,            sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.startPosition,         d_startPosition + id,       sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.velocity,              d_velocity + id,            sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.pressureForce,         d_pressureForce + id,       sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.viscosityForce,        d_viscosityForce + id,      sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.surfaceTensionForce,   d_surfaceTensionForce + id, sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.properties,            d_properties + id,          sizeof(glm::vec4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.flags,                 d_flags + id,               sizeof(glm::vec4), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host.color,                 d_color + id,               sizeof(glm::vec3), cudaMemcpyDeviceToHost);
}
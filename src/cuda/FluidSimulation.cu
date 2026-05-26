#include "FluidSimulation.h"
#include "simulation/FluidSimulation.h"
#include "cuda/Kernels.cuh"
#include "cuda/Math.cuh"

#include <complex>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <cub/cub.cuh>

#include "rendering/FluidRenderer.h"

void queryRequiredSortingStorage(
    const int numParticles,
    size_t &sortTempStorageBytes,
    const int* particleHashes,
    int* sortedParticleHashes,
    const int* particleIndices,
    int* sortedParticleIndices
) {
    cub::DeviceRadixSort::SortPairs(
        nullptr,
        sortTempStorageBytes,
        particleHashes, sortedParticleHashes,
        particleIndices, sortedParticleIndices, numParticles
    );
}

__device__ int floorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;

    if ((r != 0) && ((r < 0) != (b < 0)))
        q--;

    return q;
}

__device__ uint floorMod(int a, int b) {
    int r = a % b;

    if ((r != 0) && ((r < 0) != (b < 0)))
        r += b;

    return (uint)r;
}

// From SebLague. Find better explanation for paper
__device__ uint hashCell(int cellX, int cellY, int cellZ, int hashTableSize) {
    const uint blockSize = 50;
    const uint hash1 = 15823;
    const uint hash2 = 9737333;
    const uint hash3 = 440817757;

    // Calculate local position within the current block
    uint3 localCell = make_uint3(
        floorMod(cellX, blockSize),
        floorMod(cellY, blockSize),
        floorMod(cellZ, blockSize)
    );

    // Calculate coordinate of current block itself
    int3 blockID = make_int3(
        floorDiv(cellX, blockSize),
        floorDiv(cellY, blockSize),
        floorDiv(cellZ, blockSize)
    );

    uint blockHash = blockID.x * hash1 + blockID.y * hash2 + blockID.z * hash3;
    return (localCell.x + blockSize * (localCell.y + blockSize * localCell.z) + blockHash) % hashTableSize;
}

__global__ void hashParticles(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleHashValue,
    int* particleIndex,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int cellX = floor(particles[id].position.x / h);
    int cellY = floor(particles[id].position.y / h);
    int cellZ = floor(particles[id].position.z / h);

    uint hash = hashCell(cellX, cellY, cellZ, numParticles);

    particleHashValue[id] = hash;
    particleIndex[id] = id;
}

void launchHashParticles(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleHashValue,
    int* particleIndex,
    const float h
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    hashParticles<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleHashValue, particleIndex, h
    );
}

void sortParticlesByHash(
    const int numParticles,
    void* sortTempStorage,
    size_t sortTempStorageBytes,
    const int* particleHashes,
    int* sortedParticleHashes,
    const int* particleIndices,
    int* sortedParticleIndices
) {
    cub::DeviceRadixSort::SortPairs(
        sortTempStorage,
        sortTempStorageBytes,
        particleHashes, sortedParticleHashes,
        particleIndices, sortedParticleIndices, numParticles
    );
}

__global__ void buildOffsetsKernel(
    const int numParticles,
    const int* hashes,
    int* offsets
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    const uint hash = hashes[id];
    const uint prevHash = id == 0 ? numParticles : hashes[id - 1];

    if (hash != prevHash) {
        offsets[hash] = id;
    }
}

void launchBuildOffsetsKernel(
    const int numParticles,
    const int* sortedHashes,
    int* offsets
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    buildOffsetsKernel<<<blocks, threadsPerBlock>>>(
        numParticles, sortedHashes, offsets
    );
}

__global__ void reorderParticlesKernel(
    const int numParticles,
    const ParticleGPU* particles,
    const int* particleIndices,
    ParticleGPU* sortedParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    if (id >= numParticles) return;

    // Move particle to its sorted index
    sortedParticles[id] = particles[particleIndices[id]];
}

void launchReorderParticlesKernel(
    const int numParticles,
    const ParticleGPU* particles,
    const int* particleIndices,
    ParticleGPU* sortedParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    reorderParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndices, sortedParticles
    );
}

__device__ float3 TT(float3 min, float3 max, float3 position) {
    float3 s = max - min;
    float3 t = 2.0f * position - (max + min);

    return make_float3(
        t.x / s.x,
        t.y / s.y,
        t.z / s.z
    );
}


enum NodeType { CORNER, EDGE_XI, EDGE_ETA, EDGE_ZETA };

struct HexNode {
    float xi, eta, zeta;
    NodeType type;
};

__device__ float sshapeFunction(
    float xi,   float eta,   float zeta,        // Query positon
    float xi_i, float eta_i, float zeta_i,      // Node coords [-1, 1]
    NodeType type
) {
    switch (type) {
    case CORNER:
        // Equation 9
        return (1.0f / 64.0f)
            * (1.0f + xi_i   * xi)
            * (1.0f + eta_i  * eta)
            * (1.0f + zeta_i * zeta)
            * (9.0f * (xi*xi + eta*eta + zeta*zeta) - 19.0f);

    case EDGE_XI:
        // Equation 10
        return (9.0f / 64.0f)
            * (1.0f - xi*xi)
            * (1.0f + 9.0f * xi_i * xi)
            * (1.0f + eta_i  * eta)
            * (1.0f + zeta_i * zeta);

    case EDGE_ETA:
        // Equation 10
        return (9.0f / 64.0f)
            * (1.0f - eta*eta)
            * (1.0f + 9.0f * eta_i * eta)
            * (1.0f + xi_i  * xi)
            * (1.0f + zeta_i * zeta);

    case EDGE_ZETA:
        // Equation 10
        return (9.0f / 64.0f)
            * (1.0f - zeta*zeta)
            * (1.0f + 9.0f * zeta_i * zeta)
            * (1.0f + xi_i  * xi)
            * (1.0f + eta_i * eta);
    }
}


__constant__ HexNode hex32Nodes[32] = {
    // Corner nodes
    { -1.0f, -1.0f, -1.0f, CORNER },
    {  1.0f, -1.0f, -1.0f, CORNER },
    {  1.0f,  1.0f, -1.0f, CORNER },
    { -1.0f,  1.0f, -1.0f, CORNER },
    { -1.0f, -1.0f,  1.0f, CORNER },
    {  1.0f, -1.0f,  1.0f, CORNER },
    {  1.0f,  1.0f,  1.0f, CORNER },
    { -1.0f,  1.0f,  1.0f, CORNER },

    // Edge nodes along xi
    { -1.0f/3.0f, -1.0f, -1.0f, EDGE_XI },
    {  1.0f/3.0f, -1.0f, -1.0f, EDGE_XI },
    { -1.0f/3.0f,  1.0f, -1.0f, EDGE_XI },
    {  1.0f/3.0f,  1.0f, -1.0f, EDGE_XI },
    { -1.0f/3.0f, -1.0f,  1.0f, EDGE_XI },
    {  1.0f/3.0f, -1.0f,  1.0f, EDGE_XI },
    { -1.0f/3.0f,  1.0f,  1.0f, EDGE_XI },
    {  1.0f/3.0f,  1.0f,  1.0f, EDGE_XI },

    // Edge nodes along eta
    { -1.0f, -1.0/3.0f, -1.0f, EDGE_ETA },
    {  1.0f, -1.0/3.0f, -1.0f, EDGE_ETA },
    { -1.0f,  1.0/3.0f, -1.0f, EDGE_ETA },
    {  1.0f,  1.0/3.0f, -1.0f, EDGE_ETA },
    { -1.0f, -1.0/3.0f,  1.0f, EDGE_ETA },
    {  1.0f, -1.0/3.0f,  1.0f, EDGE_ETA },
    { -1.0f,  1.0/3.0f,  1.0f, EDGE_ETA },
    {  1.0f,  1.0/3.0f,  1.0f, EDGE_ETA },

    // Edge nodes along zeta
    { -1.0f, -1.0f, -1.0/3.0f, EDGE_ZETA },
    {  1.0f, -1.0f, -1.0/3.0f, EDGE_ZETA },
    { -1.0f,  1.0f, -1.0/3.0f, EDGE_ZETA },
    {  1.0f,  1.0f, -1.0/3.0f, EDGE_ZETA },
    { -1.0f, -1.0f,  1.0/3.0f, EDGE_ZETA },
    {  1.0f, -1.0f,  1.0/3.0f, EDGE_ZETA },
    { -1.0f,  1.0f,  1.0/3.0f, EDGE_ZETA },
    {  1.0f,  1.0f,  1.0/3.0f, EDGE_ZETA },
};

__device__ float iinterpolate(
    float xi, float eta, float zeta,
    float* nodeValues
) {
    float result = 0;

    for (int i = 0 ; i < 32 ; i++) {
        float value = sshapeFunction(
            xi, eta, zeta,
            hex32Nodes[i].xi, hex32Nodes[i].eta, hex32Nodes[i].zeta,
            hex32Nodes[i].type
        );

        result += value * nodeValues[i];
    }

    return result;
}

__device__ float ggamma(float restDensity, float h, float x) {
    if (x <= h)
        return restDensity * (1 - x / h);
    return 0;
}

__device__ float domainBoundaryDensity(float3 pos, CudaAABB domain, float restDensity, float h) {
    float density = 0.0f;

    // Distance to each face (positive = inside domain)
    float dxMin = pos.x - domain.min.x;
    float dxMax = domain.max.x - pos.x;
    float dyMin = pos.y - domain.min.y;
    float dyMax = domain.max.y - pos.y;
    float dzMin = pos.z - domain.min.z;
    float dzMax = domain.max.z - pos.z;

    // For a planar boundary the density contribution has an analytic form
    // gamma(phi) convolved with W along the half-space
    // Approximated as the same gamma ramp you use for mesh boundaries
    density += ggamma(restDensity, h, dxMin);
    density += ggamma(restDensity, h, dxMax);
    density += ggamma(restDensity, h, dyMin);
    density += ggamma(restDensity, h, dyMax);
    density += ggamma(restDensity, h, dzMin);
    density += ggamma(restDensity, h, dzMax);

    return density;
}

__global__ void densityPressureKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field boundaryField,
    const CudaAABB domain,
    const float h,
    const float k,
    const float p_0
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    float4 pos = particles[id].position;

    // Get cell of particle
    int cellX = floor(pos.x / h);
    int cellY = floor(pos.y / h);
    int cellZ = floor(pos.z / h);

    float density = 0.0f;
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cellX + dx, cellY + dy, cellZ + dz, numParticles);
                int offset = offsets[hash];

                if (offset == -1)
                    continue;

                while (offset < numParticles) {
                    int neighborIndex = offset;
                    offset++;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    const ParticleGPU other = particles[neighborIndex];
                    const float4 diff = pos - other.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        density += other.properties.x * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    if (hasBoundary) {
        // Boundary density
        float3 bCellSize = make_float3(
            (domain.max.x - domain.min.x) / boundaryField.resolution.x,
            (domain.max.y - domain.min.y) / boundaryField.resolution.y,
            (domain.max.z - domain.min.z) / boundaryField.resolution.z
        );

        int bCellX = min((int)((pos.x - domain.min.x) / bCellSize.x), boundaryField.resolution.x - 1);
        int bCellY = min((int)((pos.y - domain.min.y) / bCellSize.y), boundaryField.resolution.y - 1);
        int bCellZ = min((int)((pos.z - domain.min.z) / bCellSize.z), boundaryField.resolution.z - 1);

        // Clamp to valid range
        bCellX = max(0, bCellX);
        bCellY = max(0, bCellY);
        bCellZ = max(0, bCellZ);

        int bCellId = bCellX
                + bCellY * boundaryField.resolution.x
                + bCellZ * boundaryField.resolution.x * boundaryField.resolution.y;

        Hex32Cell* bCell = &boundaryField.d_cells[bCellId];

        if (bCell->used) {
            // Cell bounds in world space
            float3 bCellBase = make_float3(
                domain.min.x + bCellX * bCellSize.x,
                domain.min.y + bCellY * bCellSize.y,
                domain.min.z + bCellZ * bCellSize.z
            );

            float3 bCellMax = make_float3(
                bCellBase.x + bCellSize.x,
                bCellBase.y + bCellSize.y,
                bCellBase.z + bCellSize.z
            );

            float3 clampedPos = make_float3(
                fmaxf(domain.min.x, fminf(domain.max.x, pos.x)),
                fmaxf(domain.min.y, fminf(domain.max.y, pos.y)),
                fmaxf(domain.min.z, fminf(domain.max.z, pos.z))
            );

            // Map particle to natural coords [-1,1] within its boundary cell
            float3 naturalPos = TT(bCellBase, bCellMax, clampedPos);
            naturalPos.x = fmaxf(-1.0f, fminf(1.0f, naturalPos.x));
            naturalPos.y = fmaxf(-1.0f, fminf(1.0f, naturalPos.y));
            naturalPos.z = fmaxf(-1.0f, fminf(1.0f, naturalPos.z));

            float boundaryDensity = iinterpolate(naturalPos.x, naturalPos.y, naturalPos.z, bCell->nodes);

            // if (boundaryDensity < 0.0f) {
            // printf("%f\n", boundaryDensity);
            // }

            // Add to total density
            // if (boundaryDensity > 0.0) {
                // printf("%f\n", boundaryDensity);
                density += boundaryDensity;
            // }
            // particles[id].boundary.x = boundaryDensity;
        }

        // density += domainBoundaryDensity(make_float3(pos.x, pos.y, pos.z), domain, 1000, h);
        // particles[id].properties.z = k * (density - p_0);
    }

    particles[id].properties.y = density;

    const float B = (k * p_0) / 7;
    particles[id].properties.z = fmaxf(0.0f, B * (pow((density / p_0), 7) - 1));
}

void launchDensityPressureKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field boundaryField,
    const CudaAABB domain,
    const float h,
    const float k,
    const float p_0
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, hashes, offsets, hasBoundary, boundaryField, domain, h, k, p_0
    );
}

__device__ float3 shapeFunctionGradient(
    float xi,   float eta,   float zeta,
    float xi_i, float eta_i, float zeta_i,
    NodeType type
) {
    float3 grad = make_float3(0, 0, 0);

    switch (type) {
        case CORNER: {
            float A = (1.0f + xi_i*xi);
            float B = (1.0f + eta_i*eta);
            float C = (1.0f + zeta_i*zeta);
            float Q = 9.0f*(xi*xi + eta*eta + zeta*zeta) - 19.0f;

            grad.x = (1.0f/64.0f) * (xi_i*B*C*Q + A*B*C*18.0f*xi);
            grad.y = (1.0f/64.0f) * (A*eta_i*C*Q + A*B*C*18.0f*eta);
            grad.z = (1.0f/64.0f) * (A*B*zeta_i*Q + A*B*C*18.0f*zeta);
            break;
        }
        case EDGE_XI: {
            float A = (1.0f - xi*xi);
            float B = (1.0f + 9.0f*xi_i*xi);
            float C = (1.0f + eta_i*eta);
            float D = (1.0f + zeta_i*zeta);

            grad.x = (9.0f/64.0f) * ((-2.0f*xi)*B*C*D + A*9.0f*xi_i*C*D);
            grad.y = (9.0f/64.0f) * A*B*(eta_i)*D;
            grad.z = (9.0f/64.0f) * A*B*C*(zeta_i);
            break;
        }
        case EDGE_ETA: {
            float A = (1.0f - eta*eta);
            float B = (1.0f + 9.0f*eta_i*eta);
            float C = (1.0f + xi_i*xi);
            float D = (1.0f + zeta_i*zeta);

            grad.x = (9.0f/64.0f) * A*B*(xi_i)*D;
            grad.y = (9.0f/64.0f) * ((-2.0f*eta)*B*C*D + A*9.0f*eta_i*C*D);
            grad.z = (9.0f/64.0f) * A*B*C*(zeta_i);
            break;
        }
        case EDGE_ZETA: {
            float A = (1.0f - zeta*zeta);
            float B = (1.0f + 9.0f*zeta_i*zeta);
            float C = (1.0f + xi_i*xi);
            float D = (1.0f + eta_i*eta);

            grad.x = (9.0f/64.0f) * A*B*(xi_i)*D;
            grad.y = (9.0f/64.0f) * A*B*C*(eta_i);
            grad.z = (9.0f/64.0f) * ((-2.0f*zeta)*B*C*D + A*9.0f*zeta_i*C*D);
            break;
        }
    }
    return grad;
}

__device__ float3 interpolateGradient(
    float xi, float eta, float zeta,    // natural coords [-1,1]
    float3 cellMin, float3 cellMax,     // world space cell bounds
    float* nodeValues                   // ρB,i
) {
    // Jacobian of T^-1: dxi/dx = 2 / (cellMax - cellMin)
    float3 dXi_dX = make_float3(
        2.0f / (cellMax.x - cellMin.x),
        2.0f / (cellMax.y - cellMin.y),
        2.0f / (cellMax.z - cellMin.z)
    );

    float3 grad = make_float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 32; i++) {
        HexNode node = hex32Nodes[i];
        float rhoB_i = nodeValues[i];

        // Gradient of shape function in isoparametric space
        float3 dN_diso = shapeFunctionGradient(
            xi, eta, zeta,
            node.xi, node.eta, node.zeta,
            node.type
        );

        // Chain rule: dN/dx = dN/dxi * dxi/dx etc.
        grad.x += dN_diso.x * dXi_dX.x * rhoB_i;
        grad.y += dN_diso.y * dXi_dX.y * rhoB_i;
        grad.z += dN_diso.z * dXi_dX.z * rhoB_i;
    }

    return grad;
}

__global__ void sphForcesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field boundaryField,
    const CudaAABB domain,
    const float h,
    const float alpha,
    const float speedSound,
    const float tension
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // int pi = particleIndex[id];
    ParticleGPU* particle = &particles[id];

    float4 pos = particle->position;

    // Get cell of particle
    int cellX = floor(pos.x / h);
    int cellY = floor(pos.y / h);
    int cellZ = floor(pos.z / h);

    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4 n = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float c = 0.0f;

    float h2 = h * h;

    float t = 0.0;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cellX + dx, cellY + dy, cellZ + dz, numParticles);
                int offset = offsets[hash];

                if (offset == -1)
                    continue;

                while (offset < numParticles) {
                    int neighborIndex = offset;
                    offset++;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    // Skip self
                    if (neighborIndex == id)
                        continue;

                    const ParticleGPU other = particles[neighborIndex];
                    const float4 diff = pos - other.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        // Pressure
                        const float scalar =
                            other.properties.x *
                            (particle->properties.z / (particle->properties.y * particle->properties.y + 1e-6f) +
                             other.properties.z / (other.properties.y * other.properties.y + 1e-6f));

                        particle->pressureForce += scalar * spikyKernelGradient(diff, h);

                        // Viscosity
                        float4 vab = particle->velocity - other.velocity;

                        float dotTerm = dot(vab, diff);

                        if (dotTerm < 0) {
                            const float eps = 0.01f;

                            const float nu =
                                (2.0f * alpha * h * speedSound) /
                                (particle->properties.y + other.properties.y + 1e-6f);

                            const float pi_ab =
                                -nu * (dotTerm / (distSq + eps * h2));

                            particle->viscosityForce += other.properties.x * pi_ab * spikyKernelGradient(diff, h);
                        }

                        // Surface Tension
                        const float4 normDir = diff / sqrtf(distSq);
                        particle->surfaceTensionForce += other.properties.x * poly6Kernel(diff, h) * normDir;

                        // OLD STUFF
                        // float invDensity = 1.0f / (other.properties.y + 1e-6f);
                        //
                        // // Pressure
                        // // const float scalar = other.properties.x * (particle->properties.z + other.properties.z) * 0.5f * invDensity;
                        // const float scalar =
                        //     other.properties.x *
                        //     (particle->properties.z / (particle->properties.y * particle->properties.y + 1e-6f) +
                        //      other.properties.z / (other.properties.y * other.properties.y + 1e-6f));
                        // // particle->pressureForce += scalar * spikyKernelGradient(diff, h);
                        //
                        // // const float scalar = other.properties.x * (particle->properties.z + other.properties.z) / (2.0 * other.properties.y);
                        // particle->pressureForce += scalar * spikyKernelGradient(diff, h);
                        //
                        // // Viscosity
                        // const float4 viscTerm = other.properties.x * (other.velocity - particle->velocity) * invDensity;
                        // particle->viscosityForce += viscosityLaplacianKernel(diff, h) * viscTerm;
                        //
                        //  // Surface tension
                        // n += other.properties.x * invDensity * poly6KernelGradient(diff, h);
                        // c += other.properties.x * invDensity * poly6KernelLaplacian(diff, h);
                    }
                }
            }
        }
    }

    particle->pressureForce = -1.0f * particle->pressureForce;
    particle->viscosityForce = -1.0f * particle->viscosityForce;

    // float lenN = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    // if (lenN > tensionThreshold) {
    //     const float4 k = -1.0f * c / lenN;
    //     particle->surfaceTensionForce = -1.0f * t * (k / particle->properties.x);
    //     particle->flags.x = 1.0f;
    // }

    particle->surfaceTensionForce = -1.0f * (tension / particle->properties.x) * particle->surfaceTensionForce;

    if (hasBoundary) {
        float3 bCellSize = make_float3(
            (domain.max.x - domain.min.x) / boundaryField.resolution.x,
            (domain.max.y - domain.min.y) / boundaryField.resolution.y,
            (domain.max.z - domain.min.z) / boundaryField.resolution.z
        );

        int bCellX = min((int)((pos.x - domain.min.x) / bCellSize.x), boundaryField.resolution.x - 1);
        int bCellY = min((int)((pos.y - domain.min.y) / bCellSize.y), boundaryField.resolution.y - 1);
        int bCellZ = min((int)((pos.z - domain.min.z) / bCellSize.z), boundaryField.resolution.z - 1);

        // Clamp to valid range
        bCellX = max(0, bCellX);
        bCellY = max(0, bCellY);
        bCellZ = max(0, bCellZ);

        int bCellId = bCellX
            + bCellY * boundaryField.resolution.x
            + bCellZ * boundaryField.resolution.x * boundaryField.resolution.y;

        Hex32Cell* bCell = &boundaryField.d_cells[bCellId];

        if (bCell->used) {
            float3 bCellBase = make_float3(
                domain.min.x + bCellX * bCellSize.x,
                domain.min.y + bCellY * bCellSize.y,
                domain.min.z + bCellZ * bCellSize.z
            );

            float3 bCellMax = make_float3(
                bCellBase.x + bCellSize.x,
                bCellBase.y + bCellSize.y,
                bCellBase.z + bCellSize.z
            );

            float3 naturalPos = TT(bCellBase, bCellMax, make_float3(pos.x, pos.y, pos.z));
            naturalPos.x = fmaxf(-1.0f, fminf(1.0f, naturalPos.x));
            naturalPos.y = fmaxf(-1.0f, fminf(1.0f, naturalPos.y));
            naturalPos.z = fmaxf(-1.0f, fminf(1.0f, naturalPos.z));

            float3 gradient = interpolateGradient(
                naturalPos.x, naturalPos.y, naturalPos.z,
                bCellBase, bCellMax,
                bCell->nodes
            );



            float clampedPressure = fmaxf(0.0f, particle->properties.z);

            float fluidTerm = clampedPressure / (particle->properties.y * particle->properties.y + 1e-6f);
            // float boundaryTerm = particle->boundary.y / (1000 * 1000);

            // // 5. Compute the final force scaled correctly by fluid mass
            float3 fBoundary = -fluidTerm * gradient;
            //
            // // Apply directly—no arbitrary '1000.0f' scalar needed!
            particle->pressureForce.x += fBoundary.x;
            particle->pressureForce.y += fBoundary.y;
            particle->pressureForce.z += fBoundary.z;

            // float3 fBoundary =
            //     -particle->properties.x * (clampedPressure / (particle->properties.y * particle->properties.y)) * gradient;
            //
            // particle->pressureForce += make_float4(
            //     fBoundary.x * 1000.0f,
            //     fBoundary.y * 1000.0f,
            //     fBoundary.z * 1000.0f,
            //     0.0f
            // );

            // float gradMag = sqrtf(gradient.x*gradient.x +
                          // gradient.y*gradient.y +
                          // gradient.z*gradient.z);
            // printf("boundaryDensity=%.2f gradMag=%.6f\n", particle->properties.y, gradMag);

            // float scale = particle->properties.x * particle->properties.z / (particle->properties.y * particle->properties.y + 1e-6f);
            // printf("%f\n", scale);
            // scale = 1.0f;
            // particle->pressureForce.x -= scale * gradient.x;
            // particle->pressureForce.y -= scale * gradient.y;
            // particle->pressureForce.z -= scale * gradient.z;

            // Mirror the SPlisHSPlasH formulation
            // float dpi = particle->properties.z /
            //            (particle->properties.y * particle->properties.y);
            // float dpj = 0.0f;
            //
            // float scale = 1000 * (dpi + dpj);
            //
            // particle->pressureForce.x -= scale * gradient.x;
            // particle->pressureForce.y -= scale * gradient.y;
            // particle->pressureForce.z -= scale * gradient.z;
        }
    }


    // Interpolate boundary density gradient from precomputed node gradients
    // float3 gradRhoB = make_float3(0.0f, 0.0f, 0.0f);
    // for (int i = 0; i < 32; i++) {
    //     float N = sshapeFunction(
    //         naturalPos.x, naturalPos.y, naturalPos.z,
    //         hex32Nodes[i].xi, hex32Nodes[i].eta, hex32Nodes[i].zeta,
    //         hex32Nodes[i].type
    //     );
    //     gradRhoB.x += N * bCell->nodeGradients[i].x;
    //     gradRhoB.y += N * bCell->nodeGradients[i].y;
    //     gradRhoB.z += N * bCell->nodeGradients[i].z;
    // }
    //
    // // Boundary pressure force: same formulation as fluid pressure
    // // p = k(rho - rho0), so ∇p_boundary = k * ∇rho_boundary
    // float boundaryPressure = particle->properties.z;  // current particle pressure
    // float gradP_scale = boundaryPressure / (particle->properties.y + 1e-6f);

    // particle->pressureForce.x += gradP_scale * gradRhoB.x;
    // particle->pressureForce.y += gradP_scale * gradRhoB.y;
    // particle->pressureForce.z += gradP_scale * gradRhoB.z;

    // particle->viscosityForce = mu * particle->viscosityForce;
    // particle->viscosityForce = (mu / (particle->properties.y + 1e-6f)) * particle->viscosityForce;

    // Flag for surface particle
    particle->flags.x = 0.0f;

    // float lenN = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    // if (lenN > tensionThreshold) {
    //     float4 normalizedN = n / (lenN + 1e-6f);
    //     particle->surfaceTensionForce = -1.0f * tension * c * normalizedN;
    //     particle->surfaceTensionForce = particle->surfaceTensionForce / particle->properties.x;
    //     particle->flags.x = 1.0f;
    // }
}

void launchSphForcesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const bool hasBoundary,
    const Hex32Field boundaryField,
    const CudaAABB domain,
    const float h,
    const float alpha,
    const float speedSound,
    const float tension
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, hashes, offsets, hasBoundary, boundaryField, domain, h, alpha, speedSound, tension
    );
}

struct VelocitySq {
    __device__ float operator()(const ParticleGPU& p) const {
        return p.velocity.x * p.velocity.x +
               p.velocity.y * p.velocity.y +
               p.velocity.z * p.velocity.z;
    }
};

float getMaxVelocity(
    const int numParticles,
    ParticleGPU* particles
) {
    const thrust::device_ptr<ParticleGPU> begin(particles);
    const thrust::device_ptr<ParticleGPU> end(particles + numParticles);

    const float maxVal = thrust::transform_reduce(
        begin,
        end,
        VelocitySq(),
        0.0f,
        thrust::maximum<float>()
    );

    return sqrtf(maxVal);
}

__device__ float3 toLocal(float3 p, const ForceField& f) {
    // float3 d = p - f.position;
    //
    // return make_float3(
    //     dot(d, make_float3(f.right.x,   f.up.x,   f.forward.x)),
    //     dot(d, make_float3(f.right.y,   f.up.y,   f.forward.y)),
    //     dot(d, make_float3(f.right.z,   f.up.z,   f.forward.z))
    // );

    float3 d = p - f.position;

    return make_float3(
        dot(d, f.right),
        dot(d, f.up),
        dot(d, f.forward)
    );
}

__device__ bool isInsideField(float3 worldPos, const ForceField& f) {
    float3 p = toLocal(worldPos, f);

    return (fabsf(p.x) <= f.halfExtents.x &&
            fabsf(p.y) <= f.halfExtents.y &&
            fabsf(p.z) <= f.halfExtents.z);
}

__device__ float4 toWorldForce(float4 fLocal, const ForceField& f) {
    return make_float4(
        fLocal.x * f.right.x   + fLocal.y * f.up.x   + fLocal.z * f.forward.x,
        fLocal.x * f.right.y   + fLocal.y * f.up.y   + fLocal.z * f.forward.y,
        fLocal.x * f.right.z   + fLocal.y * f.up.z   + fLocal.z * f.forward.z,
        0.0
    );
}

__global__ void calculateVelocityKernel(
    ParticleGPU* particles,
    const int numParticles,
    const ForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const float gravity
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    // Update position
    const float4 force = particle->pressureForce + particle->viscosityForce + particle->surfaceTensionForce;
    // float4 acceleration = force / particle->properties.y + make_float4(0.0f, gravity, 0.0f, 0.0f);
    float4 acceleration = force + make_float4(0.0f, gravity, 0.0f, 0.0f);

    // Check force fields
    for (int f = 0 ; f < numForceFields; f++) {
        if (isInsideField(make_float3(particle->position.x, particle->position.y, particle->position.z), forceFields[f])) {
            acceleration += toWorldForce({ forceFields[f].strength, 0.0, 0.0, 0.0 }, forceFields[f]);
        }
    }

    particle->velocity += deltaTime * acceleration;
}

void launchCalculateVelocityKernel(
    ParticleGPU* particles,
    const int numParticles,
    const ForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const float gravity
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    calculateVelocityKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles, forceFields, numForceFields, deltaTime, gravity
    );
}

__global__ void xsphKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];
    float4 pos = particles[id].position;

    // Get cell of particle
    int cellX = floor(pos.x / h);
    int cellY = floor(pos.y / h);
    int cellZ = floor(pos.z / h);

    float4 xsph = make_float4(0, 0, 0, 0);
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cellX + dx, cellY + dy, cellZ + dz, numParticles);
                int offset = offsets[hash];

                if (offset == -1)
                    continue;

                while (offset < numParticles) {
                    int neighborIndex = offset;
                    offset++;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    const ParticleGPU other = particles[neighborIndex];
                    const float4 diff = pos - other.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        xsph += 0.2f * other.properties.x * (other.velocity - particle->velocity) / other.properties.y * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    particle->xsphVelocity = particle->velocity + xsph;
}

void launchXsphKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    xsphKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, hashes, offsets, h
    );
}

__global__ void updateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const ForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const CudaAABB domain,
    const float gravity
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    // Update position
    // const float4 force = particle->pressureForce + particle->viscosityForce + particle->surfaceTensionForce;
    // float4 acceleration = force / particle->properties.y + make_float4(0.0f, gravity, 0.0f, 0.0f);
    //
    // // Check force fields
    // for (int f = 0 ; f < numForceFields; f++) {
    //     if (isInsideField(make_float3(particle->position.x, particle->position.y, particle->position.z), forceFields[f])) {
    //         acceleration += toWorldForce({ forceFields[f].strength, 0.0, 0.0, 0.0 }, forceFields[f]);
    //     }
    // }
    //
    // particle->velocity += deltaTime * acceleration;
    particle->position += deltaTime * particle->xsphVelocity;

    // Domain boundaries
    if (particle->position.x < domain.min.x) {
        particle->position.x = domain.min.x;
        particle->velocity.x *= -0.4f;
    }

    if (particle->position.x > domain.max.x) {
        particle->position.x = domain.max.x;
        particle->velocity.x *= -0.4f;
    }

    if (particle->position.y < domain.min.y) {
        particle->position.y = domain.min.y;
        particle->velocity.y *= -0.4f;
    }

    if (particle->position.y > domain.max.y) {
        particle->position.y = domain.max.y;
        particle->velocity.y *= -0.4f;
    }

    if (particle->position.z < domain.min.z) {
        particle->position.z = domain.min.z;
        particle->velocity.z *= -0.4f;
    }

    if (particle->position.z > domain.max.z) {
        particle->position.z = domain.max.z;
        particle->velocity.z *= -0.4f;
    }

    // TODO - Implement correctly
    // particle->velocity += (0.5f * deltaTime) * acceleration;
    // particle->position += deltaTime * particle->velocity;
    // particle->velocity += (0.5f * deltaTime) * acceleration;



}

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const ForceField* forceFields,
    const int numForceFields,
    const float deltaTime,
    const CudaAABB& domain,
    const float gravity
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    updateParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles, forceFields, numForceFields, deltaTime, domain, gravity
    );
}

__global__ void renderPrepareKernel(
    const int numParticles,
    const ParticleGPU* particles,
    RendererParticle* rendererParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    rendererParticles[id].position = particles[id].position;
    rendererParticles[id].velocity = particles[id].velocity;
    rendererParticles[id].properties = particles[id].properties;
    rendererParticles[id].flags = particles[id].flags;
}

void launchRenderPrepareKernel(
    const int numParticles,
    const ParticleGPU* particles,
    RendererParticle* rendererParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    renderPrepareKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, rendererParticles
    );
}

__global__ void initParticlesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int3 gridCount,
    const float4 startPosition,
    const float particleSpacing,
    const float mass
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Map id to 3d position
    const int gx = id % gridCount.x;
    const int gy = (id / gridCount.x) % gridCount.y;
    const int gz = id / (gridCount.x * gridCount.y);

    float posX = startPosition.x + gx * particleSpacing;
    float posY = startPosition.y + gy * particleSpacing;
    float posZ = startPosition.z + gz * particleSpacing;

    ParticleGPU* particle = &particles[id];
    particle->position = { posX, posY, posZ, 0.0 };
    particle->startPosition = particle->position;
    particle->velocity = { 0.0, 0.0, 0.0, 0.0 };
    particle->properties = { mass, 0.0, 0.0, 0.0 };
    particle->flags = { 0.0, 0.0, 0.0, 0.0 };
    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void launchInitParticlesKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int3 gridCount,
    const float4 startPosition,
    const float particleSpacing,
    const float mass
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    initParticlesKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, gridCount, startPosition, particleSpacing, mass
    );
}

__global__ void resetParticlesKernel(
    ParticleGPU* particles,
    const int numParticles
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];
    particle->position = particle->startPosition;
    particle->velocity = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->properties.y = 0.0f;
    particle->properties.z = 0.0f;
    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void launchResetParticlesKernel(
    ParticleGPU* particles,
    const int numParticles
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    resetParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles
    );
}

__global__ void voxelFieldKernel(
    const VoxelField field,
    const CudaAABB domain,
    const int numParticles,
    const ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;
    const int cellCount = field.resolution.x * field.resolution.y * field.resolution.z;

    if (id >= cellCount)
        return;

    // Convert index to 3d cell pos
    int x = id % field.resolution.x;
    int y = (id / field.resolution.x) % field.resolution.y;
    int z = id / (field.resolution.x * field.resolution.y);

    float3 cellSizes = make_float3(
        (domain.max.x - domain.min.x) / (field.resolution.x - 1),
        (domain.max.y - domain.min.y) / (field.resolution.y - 1),
        (domain.max.z - domain.min.z) / (field.resolution.z - 1)
    );

    float4 samplePosition = make_float4(
        domain.min.x + x * cellSizes.x,
        domain.min.y + y * cellSizes.y,
        domain.min.z + z * cellSizes.z,
        0.0
    );

    // Get cell of sample point
    int cellX = floor(samplePosition.x / h);
    int cellY = floor(samplePosition.y / h);
    int cellZ = floor(samplePosition.z / h);

    float4 n = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float c = 0.0f;
    // float h2 = field.smoothingRadius * field.smoothingRadius;
    float h2 = h * h;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                uint hash = hashCell(cellX + dx, cellY + dy, cellZ + dz, numParticles);
                int offset = offsets[hash];

                if (offset == -1)
                    continue;

                while (offset < numParticles) {
                    int neighborIndex = offset;
                    offset++;

                    uint neighborHash = hashes[neighborIndex];
                    if (neighborHash != hash)
                        break;

                    const ParticleGPU particle = particles[neighborIndex];
                    const float4 diff = samplePosition - particle.position;

                    // Check if within radius
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < h2) {
                        float invDensity = 1.0f / (particle.properties.y + 1e-6f);

                        // Color field
                        n += particle.properties.x * invDensity * poly6KernelGradient(diff, h);
                        c += particle.properties.x * invDensity * poly6Kernel(diff, h);
                    }
                }
            }
        }
    }

    field.d_color[id] = c - 0.5f;
    field.d_normal[id] = make_float3(-n.x, -n.y, -n.z);
}

void launchVoxelFieldKernel(
    const VoxelField& field,
    const CudaAABB& domain,
    const int numParticles,
    const ParticleGPU* particles,
    const int* hashes,
    const int* offsets,
    const float h
) {
    int threadsPerBlock = 256;
    int cells = field.resolution.x * field.resolution.y * field.resolution.z;
    int blocks = (cells + threadsPerBlock - 1) / threadsPerBlock;

    voxelFieldKernel<<<blocks, threadsPerBlock>>>(
        field, domain, numParticles, particles, hashes, offsets, h
    );
}
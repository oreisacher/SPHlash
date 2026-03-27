#include "FluidSimulation.h"

#include <complex>

#include "simulation/FluidSimulationGPU.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <math_constants.h>
#include <cub/cub.cuh>

// --- Operator overloads ---
#pragma region OperatorOverloads
__device__ float4 operator+(const float4& a, const float4& b) {
    return make_float4(
        a.x + b.x,
        a.y + b.y,
        0.0f,
        0.0f
    );
}

__device__ float4 operator-(const float4& a, const float4& b) {
    return make_float4(
        a.x - b.x,
        a.y - b.y,
        0.0f,
        0.0f
    );
}

__device__ float4& operator+=(float4& a, const float4& b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

__device__ float4 operator/(const float4& v, float s) {
    return make_float4(
        v.x / s,
        v.y / s,
        0.0f,
        0.0f
    );
}

__device__ float4 operator*(float s, const float4& v) {
    return make_float4(
        v.x * s,
        v.y * s,
        0.0f,
        0.0f
    );
}
#pragma endregion

// --- Kernels ---
#pragma region Kernels
__device__ float poly6Kernel2D(float2 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y;
    float h2 = h * h;

    if (r2 <= h2) {
        float x = h2 - r2;
        float h8 = h2 * h2 * h2 * h2;
        return 4.0f / (CUDART_PI_F * h8) * x * x * x;
    }

    return 0.0f;
}

__device__ float4 poly6Kernel2DGradient(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y;
    float h2 = h * h;

    if (r2 > 0 && r2 < h2) {
        float x = h2 - r2;
        float h8 = h2 * h2 * h2 * h2;
        float factor = (-24.0f / (CUDART_PI_F * h8)) * x * x;
        return make_float4(factor * dist.x, factor * dist.y, 0.0f, 0.0f);
    }

    return make_float4(0.0, 0.0, 0.0, 0.0);
}

__device__ float poly6Kernel2DLaplacian(float4 dist, float h) {
    float r2 = dist.x * dist.x + dist.y * dist.y;
    float h2 = h * h;

    if (r2 < h2) {
        float h8 = h2 * h2 * h2 * h2;
        float term1 = 24.0f / (CUDART_PI_F * h8);
        float term2 = h2 - r2;
        float term3 = 3.0f * r2 - h2;

        return term1 * term2 * term3;
    }

    return 0.0f;
}

__device__ float4 spikyKernel2DGradient(float4 dist, float h) {
    float r = sqrt(dist.x * dist.x + dist.y * dist.y);

    if (r < h) {
        float x = h - r;
        float h5 = h * h * h * h * h;
        float factor = (-30.0f / (CUDART_PI_F * h5)) * x * x;
        return make_float4(
            factor * (dist.x / r),
            factor * (dist.y / r),
            0.0f,
            0.0f
        );
    }

    return make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

__device__ float viscosityLaplacianKernel2D(float4 dist, float h) {
    float r = sqrt(dist.x * dist.x + dist.y * dist.y);

    if (r <= h) {
        float h6 = h * h * h * h * h * h;
        return (45.0f / (CUDART_PI_F * h6)) * (h - r);
    }

    return 0.0f;
}
#pragma endregion

// --- Simulation Steps ---
#pragma region SimulationSteps
__global__ void computeCellIndexKernel(
    const int numParticles,
    const ParticleGPU* particles,
    int* particleCellIndex,
    int* particleIndex,
    const float cellSize,
    const int2 gridResolution
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    // Get cell of particle
    int gx = particles[id].position.x / cellSize;
    int gy = particles[id].position.y / cellSize;

    gx = max(0, min(gx, gridResolution.x - 1));
    gy = max(0, min(gy, gridResolution.y - 1));

    int cellIndex = gx + gy * gridResolution.x;

    particleCellIndex[id] = cellIndex;
    particleIndex[id] = id;
    // particles[id].properties.w = cellIndex;
}

__global__ void findCellStartEndKernel(
    const int numParticles,
    const int* particleCellIndex,
    int* cellStart,
    int* cellEnd
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    const int cell = particleCellIndex[id];

    if (id == 0) {
        cellStart[cell] = id;
    } else {
        int prevCell = particleCellIndex[id - 1];
        if (cell != prevCell) {
            cellStart[cell] = id;
            cellEnd[prevCell] = id;
        }
    }

    if (id == numParticles - 1) {
        cellEnd[cell] = id + 1;
    }
}

__global__ void densityPressureFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int2 gridResolution,
    const float h,
    const float k,
    const float p_0
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    int pi = particleIndex[id];
    float4 pos = particles[pi].position;

    int gx = pos.x / h;
    int gy = pos.y / h;

    gx = max(0, min(gx, gridResolution.x - 1));
    gy = max(0, min(gy, gridResolution.y - 1));

    float density = 0.0f;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = gx + dx;
            int ny = gy + dy;

            if (nx < 0 || ny < 0 || nx >= gridResolution.x || ny >= gridResolution.y)
                continue;

            int neighborCell = nx + ny * gridResolution.x;

            int start = cellStart[neighborCell];

            // Cell with no particles
            if (start == -1)
                continue;

            int end = cellEnd[neighborCell];

            for (int j = start ; j < end; j++) {
                int pj = particleIndex[j];

                const float2 diff = make_float2(
                    pos.x - particles[pj].position.x,
                    pos.y - particles[pj].position.y
                );

                // Check if within radius
                float distSq = diff.x * diff.x + diff.y * diff.y;
                if (distSq < h * h) {
                    density += particles[pj].properties.x * poly6Kernel2D(diff, h);
                }
            }
        }
    }

    particles[pi].properties.y = density;
    particles[pi].properties.z = k * (density - p_0);     // Check if allowed to max here
    // particles[pi].properties.z = max(k * (density - p_0), 0.0);     // Check if allowed to max here
}

__global__ void sphForcesFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int2 gridResolution,
    const float h,
    const float mu,
    const float tensionThreshold,
    const float tension
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    int pi = particleIndex[id];
    ParticleGPU* particle = &particles[pi];

    float4 pos = particle->position;

    int gx = pos.x / h;
    int gy = pos.y / h;

    gx = max(0, min(gx, gridResolution.x - 1));
    gy = max(0, min(gy, gridResolution.y - 1));

    particle->pressureForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->viscosityForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    particle->surfaceTensionForce = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4 n = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    float c = 0.0f;

    // Find neighbors
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = gx + dx;
            int ny = gy + dy;

            if (nx < 0 || ny < 0 || nx >= gridResolution.x || ny >= gridResolution.y)
                continue;

            int neighborCell = nx + ny * gridResolution.x;

            int start = cellStart[neighborCell];

            // Cell with no particles
            if (start == -1)
                continue;

            int end = cellEnd[neighborCell];

            for (int j = start ; j < end; j++) {
                int pj = particleIndex[j];

                // Skip self
                if (pi == pj)
                    continue;

                const float4 diff = make_float4(
                    pos.x - particles[pj].position.x,
                    pos.y - particles[pj].position.y,
                    0.0f, 0.0f
                );

                // Check if within radius
                float distSq = diff.x * diff.x + diff.y * diff.y;
                if (distSq < h * h) {
                    ParticleGPU* other = &particles[pj];

                    // Pressure
                    const float scalar = other->properties.x * ((particle->properties.z + other->properties.z) / (2.0f * other->properties.y));
                    particle->pressureForce += scalar * spikyKernel2DGradient(diff, h);

                    // Viscosity
                    const float4 viscTerm = other->properties.x * ((other->velocity - particle->velocity) / other->properties.y);
                    particle->viscosityForce += viscosityLaplacianKernel2D(diff, h) * viscTerm;

                    // Surface tension
                    n += other->properties.x * (1.0f / other->properties.y) * poly6Kernel2DGradient(diff, h);
                    // n += other->properties.x * (1.0f / other->properties.y) * spikyKernel2DGradient(diff, h);
                    c += other->properties.x * (1.0f / other->properties.y) * poly6Kernel2DLaplacian(diff, h);
                    // c += other->properties.x * (1.0f / other->properties.y) * viscosityLaplacianKernel2D(diff, h);
                }
            }
        }
    }

    particle->pressureForce = -1.0f * particle->pressureForce;
    particle->viscosityForce = mu * particle->viscosityForce;
    particle->debugging.x = 0.0f;

    float lenN = sqrtf(n.x * n.x + n.y * n.y);
    if (lenN > tensionThreshold) {
        float4 normalizedN = n / (lenN + 1e-6f);
        particle->surfaceTensionForce = -1.0f * tension * c * normalizedN;
        particle->debugging.x = lenN;
    }
}

__global__ void applyMouseForceKernel(
    const int numParticles,
    ParticleGPU* particles,
    const float2 mousePosition,
    const float mouseRadius,
    const float mouseForceDirection
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    const float4 dir = make_float4(
        particle->position.x - mousePosition.x,
        particle->position.y - mousePosition.y,
        0.0f, 0.0f
    );

    const float len2 = dir.x * dir.x + dir.y * dir.y;

    if (len2 < mouseRadius * mouseRadius) {
        const float len = sqrtf(len2);
        const float4 forceDir = (len > 0.0001f) ? dir / len : make_float4(0.0f, 0.0f, 0.0f, 0.0f);
        const float falloff = 1.0f - (len / mouseRadius);
        particle->velocity += mouseForceDirection * mouseRadius * falloff * forceDir;
    }
}

__global__ void updateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const float deltaTime,
    const float xMin,
    const float xMax,
    const float yMin,
    const float yMax,
    const float gravity
) {
    const uint id = threadIdx.x + blockIdx.x * blockDim.x;

    if (id >= numParticles)
        return;

    ParticleGPU* particle = &particles[id];

    // Update position
    const float4 force = particle->pressureForce + particle->viscosityForce + particle->surfaceTensionForce;
    const float4 acceleration = force / particle->properties.y + make_float4(0.0f, gravity, 0.0f, 1.0f);

    particle->velocity += (0.5f * deltaTime) * acceleration;
    particle->position += deltaTime * particle->velocity;
    particle->velocity += (0.5f * deltaTime) * acceleration;

    // Simple boundaries
    if (particle->position.x < xMin) {
        particle->position.x = xMin;
        particle->velocity.x *= -1.0f;
    }

    if (particle->position.x > xMax) {
        particle->position.x = xMax;
        particle->velocity.x *= -1.0f;
    }

    if (particle->position.y < yMin) {
        particle->position.y = yMin;
        particle->velocity.y *= -1.0f;
    }

    if (particle->position.y > yMax) {
        particle->position.y = yMax;
        particle->velocity.y *= -1.0f;
    }
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
}
#pragma endregion

// --- Launchers ---
#pragma region Launchers
void launchComputeCellIndexKernel(
    const int numParticles,
    ParticleGPU* particles,
    int* particleCellIndex,
    int* particleIndex,
    const float cellSize,
    const int2 gridResolution
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    computeCellIndexKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleCellIndex, particleIndex, cellSize, gridResolution
    );
}

void sortIndicesByCellId(
    const int numParticles,
    int* particleCellIndex,
    int* particleIndex
) {
    thrust::sort_by_key(
        thrust::device,
        particleCellIndex,
        particleCellIndex + numParticles,
        particleIndex
    );
}

void launchFindCellStartEndKernel(
    const int numParticles,
    const int* particleCellIndex,
    int* cellStart,
    int* cellEnd
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    findCellStartEndKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particleCellIndex, cellStart, cellEnd
    );
}

void launchDensityPressureFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int2 gridResolution,
    const float h,
    const float k,
    const float p_0
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    densityPressureFixedGridKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndex, cellStart, cellEnd, gridResolution, h, k, p_0
    );
}

void launchSphForcesFixedGridKernel(
    const int numParticles,
    ParticleGPU* particles,
    const int* particleIndex,
    const int* cellStart,
    const int* cellEnd,
    const int2 gridResolution,
    const float h,
    const float mu,
    const float tensionThreshold,
    const float tension
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    sphForcesFixedGridKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, particleIndex, cellStart, cellEnd, gridResolution, h, mu, tensionThreshold, tension
    );
}

void launchApplyMouseForceKernel(
    const int numParticles,
    ParticleGPU* particles,
    const float2 mousePosition,
    const float mouseRadius,
    const float mouseForceDirection
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    applyMouseForceKernel<<<blocks, threadsPerBlock>>>(
        numParticles, particles, mousePosition, mouseRadius, mouseForceDirection
    );
}

void launchUpdateParticlesKernel(
    ParticleGPU* particles,
    const int numParticles,
    const float deltaTime,
    const float xMin,
    const float xMax,
    const float yMin,
    const float yMax,
    const float gravity
) {
    int threadsPerBlock = 256;
    int blocks = (numParticles + threadsPerBlock - 1) / threadsPerBlock;

    updateParticlesKernel<<<blocks, threadsPerBlock>>>(
        particles, numParticles, deltaTime, xMin, xMax, yMin, yMax, gravity
    );
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
#pragma endregion
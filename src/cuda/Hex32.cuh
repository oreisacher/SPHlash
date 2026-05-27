#pragma once

#include <vector_functions.h>
#include <vector_types.h>
#include <cuda/Math.cuh>

enum NodeType { CORNER, EDGE_XI, EDGE_ETA, EDGE_ZETA };

struct Hex32Node {
    float xi, eta, zeta;    // Position in isoparametric space
    float x, y, z;          // Position in local space
    NodeType type;
};

__constant__ Hex32Node Hex32CubeLookup[32] = {
    // Corner nodes
    { -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, CORNER },
    {  1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, CORNER },
    {  1.0f,  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, CORNER },
    { -1.0f,  1.0f, -1.0f, 0.0f, 1.0f, 0.0f, CORNER },
    { -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, CORNER },
    {  1.0f, -1.0f,  1.0f, 1.0f, 0.0f, 1.0f, CORNER },
    {  1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, CORNER },
    { -1.0f,  1.0f,  1.0f, 0.0f, 1.0f, 1.0f, CORNER },

    // Edge nodes along xi
    { -1.0f/3.0f, -1.0f, -1.0f, 1.0f/3.0f, 0.0f, 0.0f, EDGE_XI },
    {  1.0f/3.0f, -1.0f, -1.0f, 2.0f/3.0f, 0.0f, 0.0f, EDGE_XI },
    { -1.0f/3.0f,  1.0f, -1.0f, 1.0f/3.0f, 1.0f, 0.0f, EDGE_XI },
    {  1.0f/3.0f,  1.0f, -1.0f, 2.0f/3.0f, 1.0f, 0.0f, EDGE_XI },
    { -1.0f/3.0f, -1.0f,  1.0f, 1.0f/3.0f, 0.0f, 1.0f, EDGE_XI },
    {  1.0f/3.0f, -1.0f,  1.0f, 2.0f/3.0f, 0.0f, 1.0f, EDGE_XI },
    { -1.0f/3.0f,  1.0f,  1.0f, 1.0f/3.0f, 1.0f, 1.0f, EDGE_XI },
    {  1.0f/3.0f,  1.0f,  1.0f, 2.0f/3.0f, 1.0f, 1.0f, EDGE_XI },

    // Edge nodes along eta
    { -1.0f, -1.0/3.0f, -1.0f, 0.0f, 1.0f/3.0f, 0.0f, EDGE_ETA },
    {  1.0f, -1.0/3.0f, -1.0f, 1.0f, 1.0f/3.0f, 0.0f, EDGE_ETA },
    { -1.0f,  1.0/3.0f, -1.0f, 0.0f, 2.0f/3.0f, 0.0f, EDGE_ETA },
    {  1.0f,  1.0/3.0f, -1.0f, 1.0f, 2.0f/3.0f, 0.0f, EDGE_ETA },
    { -1.0f, -1.0/3.0f,  1.0f, 0.0f, 1.0f/3.0f, 1.0f, EDGE_ETA },
    {  1.0f, -1.0/3.0f,  1.0f, 1.0f, 1.0f/3.0f, 1.0f, EDGE_ETA },
    { -1.0f,  1.0/3.0f,  1.0f, 0.0f, 2.0f/3.0f, 1.0f, EDGE_ETA },
    {  1.0f,  1.0/3.0f,  1.0f, 1.0f, 2.0f/3.0f, 1.0f, EDGE_ETA },

    // Edge nodes along zeta
    { -1.0f, -1.0f, -1.0/3.0f, 0.0f, 0.0f, 1.0f/3.0f, EDGE_ZETA },
    {  1.0f, -1.0f, -1.0/3.0f, 1.0f, 0.0f, 1.0f/3.0f, EDGE_ZETA },
    { -1.0f,  1.0f, -1.0/3.0f, 0.0f, 1.0f, 1.0f/3.0f, EDGE_ZETA },
    {  1.0f,  1.0f, -1.0/3.0f, 1.0f, 1.0f, 1.0f/3.0f, EDGE_ZETA },
    { -1.0f, -1.0f,  1.0/3.0f, 0.0f, 0.0f, 2.0f/3.0f, EDGE_ZETA },
    {  1.0f, -1.0f,  1.0/3.0f, 1.0f, 0.0f, 2.0f/3.0f, EDGE_ZETA },
    { -1.0f,  1.0f,  1.0/3.0f, 0.0f, 1.0f, 2.0f/3.0f, EDGE_ZETA },
    {  1.0f,  1.0f,  1.0/3.0f, 1.0f, 1.0f, 2.0f/3.0f, EDGE_ZETA },
};

/*
    Maps a coordinate in world space to coordinate in the element's isoparametric space.
*/
__device__ inline float3 T(float3 min, float3 max, float3 position) {
    float3 s = max - min;
    float3 t = 2.0f * position - (max + min);

    return make_float3(
        t.x / s.x,
        t.y / s.y,
        t.z / s.z
    );
}

/*
    Determine the interpolation weight for one node by given isoparametric space.
    Weights of all 32 elements must sum to one!
*/
__device__ inline float shapeFunction(
    float xi,   float eta,   float zeta,        // Position to interpolate
    float xi_i, float eta_i, float zeta_i,      // Coordinates of hex32 node
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

        default: return 0.0;
    }
}

__device__ inline float3 shapeFunctionGradient(
    float xi,   float eta,   float zeta,        // Position to interpolate
    float xi_i, float eta_i, float zeta_i,      // Coordinates of hex32 node
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

/*
     Interpolate value inside a Hex32Cube at given position in isoparametric space.
     Passed values need to be of length 32!
*/
__device__ inline float interpolateHex32(
    float xi, float eta, float zeta,    // Position to interpolate
    const float* values                 // Values of the 32 nodes used for interpolation
) {
    float result = 0;

    for (int i = 0 ; i < 32 ; i++) {
        result += shapeFunction(
            xi, eta, zeta,
            Hex32CubeLookup[i].xi, Hex32CubeLookup[i].eta, Hex32CubeLookup[i].zeta,
            Hex32CubeLookup[i].type
        ) * values[i];
    }

    return result;
}

__device__ inline float3 interpolateHex32Gradient(
    float xi, float eta, float zeta,     // Position to interpolate
    const float* values                  // Values of the 32 nodes used for interpolation
) {
    float3 gradient = make_float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 32; i++) {
        float3 dN = shapeFunctionGradient(
            xi, eta, zeta,
            Hex32CubeLookup[i].xi, Hex32CubeLookup[i].eta, Hex32CubeLookup[i].zeta,
            Hex32CubeLookup[i].type
        );

        gradient += dN * values[i];
    }

    return gradient;
}

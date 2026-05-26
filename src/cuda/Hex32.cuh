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

__constant__ Hex32Node hex32Cube[32] = {
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
            hex32Cube[i].xi, hex32Cube[i].eta, hex32Cube[i].zeta,
            hex32Cube[i].type
        ) * values[i];
    }

    return result;
}
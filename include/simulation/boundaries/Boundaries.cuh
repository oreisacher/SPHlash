//
// Created by ruut on 4/20/26.
//

#ifndef SPHLASH_BOUNDARIES_H
#define SPHLASH_BOUNDARIES_H

struct CudaAABB;
struct Hex32Field;
struct Triangle;

__global__ void sdfKernel(
    int triangleCount,
    const Triangle* triangles,
    Hex32Field hexField,
    CudaAABB domain,
    float h
);

__global__ void volumeMapKernel(
    Hex32Field sdfField,
    Hex32Field volumeField,
    CudaAABB domain,
    float h
);

#endif //SPHLASH_BOUNDARIES_H

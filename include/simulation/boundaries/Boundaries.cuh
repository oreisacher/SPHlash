//
// Created by ruut on 4/20/26.
//

#ifndef SPHLASH_BOUNDARIES_H
#define SPHLASH_BOUNDARIES_H

struct AABBView;
struct Hex32Field;
struct Triangle;

__global__ void sdfKernel(
    int triangleCount,
    const Triangle* triangles,
    Hex32Field hexField,
    AABBView domain,
    float h
);

__global__ void volumeMapKernel(
    Hex32Field sdfField,
    Hex32Field volumeField,
    AABBView domain,
    float h
);

#endif //SPHLASH_BOUNDARIES_H

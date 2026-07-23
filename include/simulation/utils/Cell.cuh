//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_CELL_CUH
#define SPHLASH_CELL_CUH

#include "simulation/utils/Math.cuh"

__device__ inline uint hashCell(int cellX, int cellY, int cellZ, int hashTableSize) {
    constexpr uint blockSize = 50;
    constexpr uint hash1 = 15823;
    constexpr uint hash2 = 9737333;
    constexpr uint hash3 = 440817757;

    // Calculate local position within the current block
    uint3 localCell = make_uint3(
        floorMod(cellX, blockSize),
        floorMod(cellY, blockSize),
        floorMod(cellZ, blockSize)
    );

    // Calculate coordinate of current block itself
    uint3 blockID = make_uint3(
        floorDiv(cellX, blockSize),
        floorDiv(cellY, blockSize),
        floorDiv(cellZ, blockSize)
    );

    long blockHash = blockID.x * hash1 + blockID.y * hash2 + blockID.z * hash3;
    return (localCell.x + blockSize * (localCell.y + blockSize * localCell.z) + blockHash) % hashTableSize;
}

#endif //SPHLASH_CELL_CUH

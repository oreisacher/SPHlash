//
// Created by ruut on 7/23/26.
//

#ifndef SPHLASH_CELL_CUH
#define SPHLASH_CELL_CUH

__device__ inline int floorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;

    if ((r != 0) && ((r < 0) != (b < 0)))
        q--;

    return q;
}

__device__ inline uint floorMod(int a, int b) {
    int r = a % b;

    if ((r != 0) && ((r < 0) != (b < 0)))
        r += b;

    return (uint)r;
}

__device__ inline uint hashCell(glm::ivec3 cell, int hashTableSize) {
    constexpr uint blockSize = 50;
    constexpr uint hash1 = 15823;
    constexpr uint hash2 = 9737333;
    constexpr uint hash3 = 440817757;

    // Calculate local position within the current block
    glm::ivec3 localCell(
        floorMod(cell.x, blockSize),
        floorMod(cell.y, blockSize),
        floorMod(cell.z, blockSize)
    );

    // Calculate coordinate of current block itself
    glm::ivec3 blockID(
        floorDiv(cell.x, blockSize),
        floorDiv(cell.y, blockSize),
        floorDiv(cell.z, blockSize)
    );

    long blockHash = blockID.x * hash1 + blockID.y * hash2 + blockID.z * hash3;
    return (localCell.x + blockSize * (localCell.y + blockSize * localCell.z) + blockHash) % hashTableSize;
}

#endif //SPHLASH_CELL_CUH

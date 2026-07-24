//
// Created by ruut on 5/9/26.
//

#ifndef SPHLASH_HEX32FIELD_H
#define SPHLASH_HEX32FIELD_H

#include <vector_types.h>

// Corner nodes     0 - 7
// Edge nodes       8 - 31
struct Hex32Cell {
    float nodes[32];
    bool used;
};

struct Hex32Field {
    glm::ivec3 resolution = { 0, 0, 0 };
    unsigned int texture = 0;

    Hex32Cell* d_cells = nullptr;
};

#endif //SPHLASH_HEX32FIELD_H

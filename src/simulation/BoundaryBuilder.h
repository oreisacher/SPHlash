//
// Created by ruut on 5/9/26.
//

#ifndef SPHLASH_BOUNDARYBUILDER_H
#define SPHLASH_BOUNDARYBUILDER_H

#include "glm/vec3.hpp"
#include "rendering/Scene.h"
#include "utils/Hex32Field.h"

struct BoundarySettings {
    glm::ivec3 resolution = { 32, 32, 32 };
};

class BoundaryBuilder {
public:
    BoundaryBuilder();

    Hex32Field constructDensityMap(const Scene& scene, AABB domain);

    BoundarySettings settings;

    GLuint tex3D;
};

#endif //SPHLASH_BOUNDARYBUILDER_H

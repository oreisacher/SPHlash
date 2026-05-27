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
    int samples = 200;
};

class BoundaryBuilder {
public:
    Hex32Field constructDensityMap(const Scene* scene, AABB domain, float h, float restDensity) const;

    Hex32Field constructSDFMap(const Scene* scene, AABB domain) const;
    Hex32Field constructVolumeMap(Hex32Field sdfMap, AABB domain) const;

    BoundarySettings settings;
};

#endif //SPHLASH_BOUNDARYBUILDER_H

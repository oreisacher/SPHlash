//
// Created by ruut on 5/9/26.
//

#ifndef SPHLASH_BOUNDARYBUILDER_H
#define SPHLASH_BOUNDARYBUILDER_H

#include "glm/vec3.hpp"
#include "../AABB.cuh"
#include "Hex32Field.h"

class Scene;

struct BoundarySettings {
    bool buildOnInit        = true;
    glm::ivec3 resolution   = { 32, 32, 32 };
};

class BoundaryBuilder {
public:
    bool constructSDFMap(const Scene* scene, AABB domain, float h, Hex32Field& sdfMap) const;
    bool constructVolumeMap(Hex32Field sdfHexField, AABB domain, float h, Hex32Field& volumeMap) const;

    BoundarySettings settings;
};

#endif //SPHLASH_BOUNDARYBUILDER_H

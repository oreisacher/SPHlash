//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_IBOUNDED_H
#define SPHLASH_IBOUNDED_H

#include "../AABB.cuh"

class IBounded {
public:
    virtual ~IBounded() = default;
    virtual AABB getWorldBoundingBox() const = 0;
};

#endif //SPHLASH_IBOUNDED_H

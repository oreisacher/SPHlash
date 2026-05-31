//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_IRENDERABLE_H
#define SPHLASH_IRENDERABLE_H

#include "glm/fwd.hpp"

class IRenderable {
public:
    virtual ~IRenderable() = default;

    virtual void draw(const glm::mat4& view, const glm::mat4& projection) const = 0;
};

#endif //SPHLASH_IRENDERABLE_H

//
// Created by ruut on 6/13/26.
//

#ifndef SPHLASH_RENDERERPARTICLE_H
#define SPHLASH_RENDERERPARTICLE_H

struct RendererParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 properties;
    glm::vec4 flags;
    glm::vec3 color;
};

#endif //SPHLASH_RENDERERPARTICLE_H

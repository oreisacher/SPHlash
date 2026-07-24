//
// Created by ruut on 6/13/26.
//

#ifndef SPHLASH_FORCEFIELD_H
#define SPHLASH_FORCEFIELD_H

struct ForceField {
    glm::vec3 position     = { 0.0f, 0.0f, 0.0f };
    glm::vec3 halfExtents  = { 0.2f, 0.2f, 0.2f };

    glm::vec3 right        = { 1.0f, 0.0f, 0.0f };
    glm::vec3 up           = { 0.0f, 1.0f, 0.0f };
    glm::vec3 forward      = { 0.0f, 0.0f, 1.0f };

    bool active         = true;
    float strength      = 5.0f;
};

#endif //SPHLASH_FORCEFIELD_H

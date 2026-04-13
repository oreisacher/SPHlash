//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MODEL_H
#define SPHLASH_MODEL_H

#include <string>
#include "Mesh.h"
#include "Shader.h"

struct Material {
    std::string name;

    glm::vec3 albedo = { 1.0f, 1.0f, 1.0f };
};

struct Transform {
    glm::vec3 position  = { 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation  = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale     = { 1.0f, 1.0f, 1.0f };
};

class Model {
public:
    Model(const std::string &path);

    void draw(const Shader* shader) const;

    Transform* getTransform();
    void updateTransform();

    std::string getName() const;

private:
    std::string name = "Undefined";

    Transform transform;

    glm::mat4 modelMatrix = glm::mat4(1.0f);

    std::vector<Mesh*> meshes           = {};
    std::vector<Material> materials    = {};
};

#endif //SPHLASH_MODEL_H

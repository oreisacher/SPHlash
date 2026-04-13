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

class Model {
public:
    Model(const std::string &path);

    void draw(const Shader* shader) const;

private:
    std::vector<Mesh*> meshes           = {};
    std::vector<Material> materials    = {};
};

#endif //SPHLASH_MODEL_H

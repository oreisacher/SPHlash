//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MESH_H
#define SPHLASH_MESH_H

#include <string>

#include "glad/gl.h"
#include "glm/glm.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh {
public:
    Mesh(
        const std::string& name,
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices
    );

    void draw() const;

    void setMaterialIndex(int index);
    int getMaterialIndex() const;

private:
    void uploadToGPU();

    std::string name;
    int materialIndex = -1;

    std::vector<Vertex>         vertices = {};
    std::vector<unsigned int>   indices  = {};

    GLuint vao, vbo, ebo;
};

#endif //SPHLASH_MESH_H

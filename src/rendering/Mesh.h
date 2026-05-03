//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MESH_H
#define SPHLASH_MESH_H

#include <driver_types.h>
#include <memory>
#include <string>
#include <vector_types.h>

#include "glad/gl.h"
#include "glm/glm.hpp"

struct Vertex {
    float3 position;
    float3 normal;
    float2 texCoord;
};

struct Triangle {
    float3 v0, v1, v2;
};

class Mesh {
public:
    Mesh(
        const std::string& name,
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        GLenum primitiveMode = GL_TRIANGLES
    );

    Mesh(
        const std::string& name,
        int maxVertices,
        int maxIndices
    );

    ~Mesh();

    void draw() const;

    std::string getName() const;

    void updateMeshData(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    void setMaterialIndex(int index);
    int getMaterialIndex() const;

    std::vector<Triangle> getTriangles(glm::mat4 model) const;

    cudaGraphicsResource* getVerticesResource() const;
    cudaGraphicsResource* getIndicesResource() const;

    void setVertexCount(int vertexCount);
    int getVertexCount() const;
    int getMaxVertexCount() const;

    void setIndexCount(int indexCount);
    int getIndexCount() const;
    int getMaxIndexCount() const;

    // --- Factory ---
    static std::unique_ptr<Mesh> createBox(const std::string& name);
private:
    void uploadToGPU();
    void updateGPU() const;

    std::string name;
    int materialIndex = 0;

    std::vector<Vertex>         vertices = {};
    std::vector<unsigned int>   indices  = {};

    GLuint vao, vbo, ebo;
    GLenum primitiveMode                 = GL_TRIANGLES;

    int maxVertices = 0, maxIndices = 0;
    int vertexCount = 0, indexCount = 0;

    // --- CUDA Interopt ---
    cudaGraphicsResource* verticesResource = nullptr;
    cudaGraphicsResource* indicesResource  = nullptr;
};

#endif //SPHLASH_MESH_H

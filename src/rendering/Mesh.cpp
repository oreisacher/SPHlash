//
// Created by ruut on 4/13/26.
//

#include "Mesh.h"
#include <cuda_gl_interop.h>
#include <vector_functions.h>
#include "tiny_obj_loader.h"
#include "utils/Primitives.h"
#include "utils/SphereCreator.h"

Mesh::Mesh(
    const std::string& name,
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    GLenum primitiveMode
)
    : name(name), vertices(vertices), indices(indices), primitiveMode(primitiveMode)
{
    vertexCount = vertices.size();
    maxVertices = vertexCount;

    indexCount = indices.size();
    maxIndices = indexCount;

    uploadToGPU();
}

Mesh::Mesh(const std::string& name, int maxVertices, int maxIndices)
    : maxVertices(maxVertices), maxIndices(maxIndices), name(name)
{
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex Attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Register buffer with CUDA
    cudaGraphicsGLRegisterBuffer(&verticesResource, vbo, cudaGraphicsRegisterFlagsWriteDiscard);
    cudaGraphicsGLRegisterBuffer(&indicesResource, ebo, cudaGraphicsRegisterFlagsWriteDiscard);
}

Mesh::~Mesh() {
    if (verticesResource) cudaGraphicsUnregisterResource(verticesResource);
    if (indicesResource) cudaGraphicsUnregisterResource(indicesResource);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(primitiveMode, indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawInstanced(int count) const {
    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, count);
}

std::string Mesh::getName() const {
    return name;
}

void Mesh::updateMeshData(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    this->vertices = vertices;
    this->indices = indices;

    vertexCount = vertices.size();
    indexCount = indices.size();

    updateGPU();
}

void Mesh::setMaterialIndex(const int index) {
    materialIndex = index;
}

int Mesh::getMaterialIndex() const {
    return materialIndex;
}

std::vector<Triangle> Mesh::getTriangles(glm::mat4 model) const {
    // std::vector<Triangle> triangles;
    //
    // for (int i = 0; i < indices.size(); i += 3) {
    //     const glm::vec3 v0 = model * glm::vec4(vertices[indices[i    ]].position, 1.0f);
    //     const glm::vec3 v1 = model * glm::vec4(vertices[indices[i + 1]].position, 1.0f);
    //     const glm::vec3 v2 = model * glm::vec4(vertices[indices[i + 2]].position, 1.0f);
    //
    //     triangles.push_back({
    //         make_float3(v0.x, v0.y, v0.z),
    //         make_float3(v1.x, v1.y, v1.z),
    //         make_float3(v2.x, v2.y, v2.z)
    //     });
    // }
    //
    // return triangles;
}

cudaGraphicsResource* Mesh::getVerticesResource() const {
    return verticesResource;
}

cudaGraphicsResource* Mesh::getIndicesResource() const {
    return indicesResource;
}

void Mesh::setVertexCount(int vertexCount) {
    this->vertexCount = vertexCount;
}

int Mesh::getVertexCount() const {
    return vertexCount;
}

int Mesh::getMaxVertexCount() const {
    return maxVertices;
}

void Mesh::setIndexCount(int indexCount) {
    this->indexCount = indexCount;
}

int Mesh::getIndexCount() const {
    return indexCount;
}

int Mesh::getMaxIndexCount() const {
    return maxIndices;
}

GLuint Mesh::getVAO() const {
    return vao;
}

std::unique_ptr<Mesh> Mesh::createBox(const std::string& name) {
    return std::make_unique<Mesh>(
        name,
        Primitives::BoxGeometry::GetVertices(),
        Primitives::BoxGeometry::GetIndices(),
        GL_TRIANGLES
    );
}

std::unique_ptr<Mesh> Mesh::createWireframeBox(const std::string& name) {
    return std::make_unique<Mesh>(
        name,
        Primitives::BoxGeometry::GetVertices(),
        Primitives::BoxGeometry::GetWireframeIndices(),
        GL_LINES
    );
}

std::unique_ptr<Mesh> Mesh::createSphere(const std::string& name, int sectorCount, int stackCount) {
    // Generate mesh data for sphere
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    createSphereData(vertices, indices, 4, 2);

    return std::make_unique<Mesh>(
        name,
        vertices,
        indices
    );
}

void Mesh::uploadToGPU() {
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex Attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::updateGPU() const {
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}

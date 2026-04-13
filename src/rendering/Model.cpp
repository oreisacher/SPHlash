//
// Created by ruut on 4/13/26.
//

#include "Model.h"

#include <format>
#include <iostream>

#include "tiny_obj_loader.h"

Model::Model(const std::string& path) {
    // Load model from disk
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    bool success = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        nullptr,
        nullptr,
        path.c_str(),
        "assets/models"
    );

    if (!success) {
        std::cerr << std::format("Failed to load model on path '{}'", path) << std::endl;
    }

    // Parse object
    struct VertexKey {
        int v, vt, vn;

        bool operator<(const VertexKey& other) const {
            return std::tie(v, vt, vn) < std::tie(other.v, other.vt, other.vn);
        }
    };

    for (const auto& shape : shapes) {
        // Maps global VertexKey to local (per mesh) vertices index
        std::map<VertexKey, unsigned int> vertexToIndex;

        // Mesh data
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (const auto& idx : shape.mesh.indices) {
            VertexKey key {
                idx.vertex_index,
                idx.texcoord_index,
                idx.normal_index
            };

            // Check if vertex with given indices already exists
            if (!vertexToIndex.contains(key)) {
                Vertex vertex;

                // Position
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                vertex.position = { vx, vy, vz };

                // Normal
                float nx = attrib.normals[3 * idx.normal_index + 0];
                float ny = attrib.normals[3 * idx.normal_index + 1];
                float nz = attrib.normals[3 * idx.normal_index + 2];
                vertex.normal = { nx, ny, nz };

                // Texture
                float tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                float ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                vertex.texCoord = { tx, ty };

                vertexToIndex[key] = vertices.size();
                vertices.push_back(vertex);
            }

            indices.push_back(vertexToIndex[key]);
        }

        auto mesh = new Mesh(shape.name, vertices, indices);

        // Material
        if (shape.mesh.material_ids[0] != -1) {
            auto mat = materials[shape.mesh.material_ids[0]];

            Material material;
            material.name = mat.name;
            material.albedo = { mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] };

            mesh->setMaterialIndex(this->materials.size());
            this->materials.push_back(material);
        }

        meshes.push_back(mesh);
    }
}

void Model::draw(const Shader* shader) const {
    for (const auto& mesh : meshes) {
        // Set material
        int matIndex = mesh->getMaterialIndex();
        if (matIndex != -1) {
            shader->setVec3("albedo", materials[matIndex].albedo);
        }

        // Draw mesh
        mesh->draw();
    }
}

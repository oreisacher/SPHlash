//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MODEL_H
#define SPHLASH_MODEL_H

#include <string>
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "glm/detail/type_quat.hpp"

struct Transform {
    glm::vec3 position  = { 0.0f, 0.0f, 0.0f };
    glm::quat rotation  = { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale     = { 1.0f, 1.0f, 1.0f };
};

class Model {
public:
    Model(const std::string& path);
    Model(const std::string& name, std::vector<std::unique_ptr<Mesh>> meshes);

    void draw(const glm::mat4& view, const glm::mat4& projection) const;

    Transform getTransform() const;
    Transform* getTransformReference();
    void setTransform(Transform transform);
    void setTransform(glm::mat4 transform);

    void updateTransform();

    std::string getName() const;

    std::vector<Triangle> getTriangles() const;

    int addMaterial(const Material& material);

    void setVisible(bool on);
    bool isVisible() const;

private:
    std::string name = "Undefined";

    bool visible = true;

    Transform transform;
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    std::vector<std::unique_ptr<Mesh>> meshes   = {};
    std::vector<Material> materials             = {};
};

#endif //SPHLASH_MODEL_H

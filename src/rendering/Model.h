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

    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

enum class Type {
    NONE,
    BOUNDARY
};

class Model {
public:
    Model(const std::string& path, bool shaded = true);
    Model(const std::string& name, std::unique_ptr<Mesh> mesh);
    Model(const std::string& name, std::vector<std::unique_ptr<Mesh>> meshes);
    virtual ~Model() = default;

    void draw(const glm::mat4& view, const glm::mat4& projection) const;

    Transform getTransform() const;
    void setTransform(Transform transform);
    void setTransform(glm::mat4 transform);

    void updateTransform();

    void setName(const std::string& name);
    std::string getName() const;

    std::vector<Triangle> getWorldSpaceTriangles() const;

    void addMesh(std::unique_ptr<Mesh> mesh);
    std::vector<Mesh*> getMeshes() const;

    int addMaterial(const Material& material);
    Material& getMaterial(int index);

    void setVisible(bool on);
    bool isVisible() const;

    void setType(Type type);
    Type getType() const;

    void setSceneGraphDeletable(bool deletable);
    bool isSceneGraphDeletable() const;

    void setSceneGraphVisible(bool visible);
    bool isSceneGraphVisible() const;

    void setInstanceCount(int c);

    void addOnTransformChangedListener(const std::function<void(Model*)>& listener);
    void notifyOnTransformChangedListeners();

    // --- Factory ---
    static std::unique_ptr<Model> createCubeModel(const std::string& name);
protected:
    std::string name = "Undefined";

    bool visible = true;
    Type type = Type::NONE;

    bool sceneGraphVisible = true;
    bool sceneGraphDeletable = true;

    Transform transform;

    std::vector<std::unique_ptr<Mesh>> meshes   = {};
    std::vector<Material> materials             = {};

    int instanceCount = 0;

    std::vector<std::function<void(Model*)>> onTransformChangedListeners;
};

#endif //SPHLASH_MODEL_H

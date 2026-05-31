//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MODEL_H
#define SPHLASH_MODEL_H

#include <string>

#include "IRenderable.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "Transform.h"
#include "glm/detail/type_quat.hpp"
#include "ui/ISelectable.h"
#include "ui/SceneNode.h"
#include "utils/AABB.h"

enum class Type {
    NONE,
    BOUNDARY
};

class Model : public Transform, public IRenderable, public SceneNode {
public:
    Model(const std::string& path, bool shaded = true);
    Model(const std::string& name, std::unique_ptr<Mesh> mesh);
    Model(const std::string& name, std::vector<std::unique_ptr<Mesh>> meshes);

    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    // --- Transformation ---
    void setTransform(glm::mat4 transform) override;

    // --- Bounding Boxes ---
    void updateLocalBoundingBox();
    void updateWorldBoundingBox();

    std::vector<Triangle> getWorldSpaceTriangles() const;

    void addMesh(std::unique_ptr<Mesh> mesh);
    std::vector<Mesh*> getMeshes() const;

    int addMaterial(const Material& material);
    Material& getMaterial(int index);

    void setType(Type type);
    Type getType() const;

    void setSceneGraphDeletable(bool deletable);
    bool isSceneGraphDeletable() const;

    void setInstanceCount(int c);

    void addOnTransformChangedListener(const std::function<void(Model*)>& listener);
    void notifyOnTransformChangedListeners();

    Properties buildProperties() override;

    // --- Factory ---
    static std::unique_ptr<Model> createCubeModel(const std::string& name);
protected:
    bool visible = true;
    Type type = Type::NONE;

    AABB localBounds;
    AABB worldBounds;

    std::vector<std::unique_ptr<Mesh>> meshes   = {};
    std::vector<Material> materials             = {};

    int instanceCount = 0;

    std::vector<std::function<void(Model*)>> onTransformChangedListeners;
};

#endif //SPHLASH_MODEL_H

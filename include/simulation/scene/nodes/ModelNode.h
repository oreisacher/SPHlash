//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_MODELNODE_H
#define SPHLASH_MODELNODE_H

#include "../IBounded.h"
#include "../Transform.h"
#include "SceneNode.h"
#include "rendering/IRenderable.h"

class BoundingBoxNode;
struct Triangle;
class Model;

class ModelNode : public Transform, public SceneNode, public IBounded, public IRenderable  {
public:
    ModelNode(const std::string& name, const std::shared_ptr<Model>& model);
    ModelNode(const nlohmann::json json);

    void onAddedToScene() override;

    Model* getModel() const;
    std::vector<Triangle> getWorldSpaceTriangles() const;

    AABB getBoundingBox() const;
    AABB getWorldBoundingBox() const override;

    void setTransform(glm::mat4 transform) override;
    void updateTransform() override;

    // -- Boundary ---
    bool isBoundary() const;
    void setBoundary(bool on);

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;
    SceneNode* getNode() override;

protected:
    std::shared_ptr<Model> model;

    bool boundary = true;
    AABB worldBoundingBox;

    BoundingBoxNode* boundingBox;
};

#endif //SPHLASH_MODELNODE_H

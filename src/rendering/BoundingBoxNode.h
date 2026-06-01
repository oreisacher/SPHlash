//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_AREAMODEL_H
#define SPHLASH_AREAMODEL_H

#include "IBounded.h"
#include "Model.h"
#include "ui/ISelectable.h"
#include "ui/SceneNode.h"
#include "utils/AABB.h"

class BoundingBoxNode : public Transform, public SceneNode, public IBounded {
public:
    BoundingBoxNode(const std::string& name, glm::vec3 color);

    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    void setTransform(glm::mat4 transform) override;
    void setTransform(const AABB& boundingBox);

    void setTarget(IBounded* target);
    void clearTarget();

    AABB getWorldBoundingBox() const override;
    CudaAABB getCudaAABB() const;

    Properties buildProperties() override;

private:
    bool visible = true;

    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Material> material;

    IBounded* target = nullptr;
};

#endif //SPHLASH_AREAMODEL_H

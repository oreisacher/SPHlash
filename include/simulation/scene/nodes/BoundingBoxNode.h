//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_AREAMODEL_H
#define SPHLASH_AREAMODEL_H

#include "../IBounded.h"
#include "../../../assets/Material.h"
#include "../../../assets/Mesh.h"
#include "../Transform.h"
#include "SceneNode.h"

class BoundingBoxNode : public Transform, public SceneNode, public IBounded {
public:
    BoundingBoxNode(const std::string& name, glm::vec3 color);
    BoundingBoxNode(const nlohmann::json json);

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    void setTransform(glm::mat4 transform) override;
    void setTransform(const AABBView& boundingBox);

    void setTarget(IBounded* target);
    void clearTarget();

    AABB getWorldBoundingBox() const override;

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

protected:
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Material> material;

    glm::vec3 color;

    IBounded* target = nullptr;
};

#endif //SPHLASH_AREAMODEL_H

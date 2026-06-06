//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_FORCEMODEL_H
#define SPHLASH_FORCEMODEL_H

#include <vector_types.h>

#include "Model.h"
#include "Transform.h"
#include "ui/SceneNode.h"

struct CudaForceField {
    float3 position     = { 0.0f, 0.0f, 0.0f };
    float3 halfExtents  = { 0.2f, 0.2f, 0.2f };

    float3 right        = { 1.0f, 0.0f, 0.0f };
    float3 up           = { 0.0f, 1.0f, 0.0f };
    float3 forward      = { 0.0f, 0.0f, 1.0f };

    bool active         = true;
    float strength      = 5.0f;
};

class ForceFieldNode : public Transform, public SceneNode {
public:
    ForceFieldNode(const std::string& name);
    explicit ForceFieldNode(const nlohmann::json json);

    CudaForceField getForceField() const;

    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    void setTransform(glm::mat4 transform) override;
    void updateTransform() override;

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    std::unique_ptr<Model> model;
    CudaForceField forceField;
};

#endif //SPHLASH_FORCEMODEL_H

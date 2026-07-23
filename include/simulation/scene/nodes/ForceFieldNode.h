//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_FORCEMODEL_H
#define SPHLASH_FORCEMODEL_H

#include "../../../assets/Model.h"
#include "../Transform.h"
#include "SceneNode.h"
#include "cuda/CudaForceField.h"

class ForceFieldNode : public Transform, public SceneNode {
public:
    ForceFieldNode(const std::string& name);
    explicit ForceFieldNode(const nlohmann::json json);

    CudaForceField getForceField() const;

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    void setTransform(glm::mat4 transform) override;
    void updateTransform() override;

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    std::unique_ptr<Model> model;
    std::unique_ptr<Model> wireframeModel;
    CudaForceField forceField;
};

#endif //SPHLASH_FORCEMODEL_H

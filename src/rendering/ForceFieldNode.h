//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_FORCEMODEL_H
#define SPHLASH_FORCEMODEL_H

#include "Model.h"
#include "simulation/FluidSimulation.h"

class ForceFieldNode : public Transform, public IRenderable, public SceneNode {
public:
    ForceFieldNode(const std::string& name);

    CudaForceField getForceField() const;

    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    void setTransform(glm::mat4 transform) override;
    void updateTransform() override;

    Properties buildProperties() override;

private:
    std::unique_ptr<Model> model;
    CudaForceField forceField;
};

#endif //SPHLASH_FORCEMODEL_H

//
// Created by ruut on 6/6/26.
//

#ifndef SPHLASH_SKYBOXNODE_H
#define SPHLASH_SKYBOXNODE_H

#include "assets/Model.h"
#include "assets/Texture.h"
#include "SceneNode.h"

class SkyboxNode : public SceneNode {
public:
    SkyboxNode();

    Properties buildProperties() override;

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    int getRenderPriority() const override;

private:
    std::shared_ptr<Model> skyboxModel;
    Shader* skyboxShader;
    std::shared_ptr<Texture> skyboxTexture;

    glm::mat4 skyboxPerspective;
};

#endif //SPHLASH_SKYBOXNODE_H

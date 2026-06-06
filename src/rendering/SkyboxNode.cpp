//
// Created by ruut on 6/6/26.
//

#include "SkyboxNode.h"

#include <iostream>

SkyboxNode::SkyboxNode() {
    name = "Skybox";
    sceneGraphVisible = false;
    savable = false;

    // --- Skybox Model ---
    skyboxModel = std::make_shared<Model>("Skybox", Mesh::createSphere("Skybox Mesh", 10, 10));

    skyboxTexture = std::make_shared<Texture>("assets/textures/skybox.png");

    Material skyboxMaterial("Skybox", Shader::createSkybox());
    skyboxMaterial.setTexture("skybox", skyboxTexture);
    this->skyboxShader = skyboxMaterial.getShader();
    skyboxModel->addMaterial(skyboxMaterial);
}

Properties SkyboxNode::buildProperties() {
    return {};
}

void SkyboxNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    glFrontFace(GL_CW);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    skyboxModel->draw(view, projection, glm::mat4(1.0f));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
}

int SkyboxNode::getRenderPriority() const {
    return -1;
}

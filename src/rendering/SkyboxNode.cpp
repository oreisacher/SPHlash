//
// Created by ruut on 6/6/26.
//

#include "SkyboxNode.h"

#include <X11/Xlib.h>

SkyboxNode::SkyboxNode() {
    name = "Skybox";
    deletable = false;
    savable = false;

    // --- Skybox Model ---
    skyboxModel = std::make_shared<Model>("Skybox", Mesh::createSphere("Skybox Mesh", 10, 10));

    skyboxTexture = std::make_shared<Texture>("assets/textures/skybox1.png");

    Material skyboxMaterial("Skybox", Shader::createSkybox());
    skyboxMaterial.setTexture("skybox", skyboxTexture);
    this->skyboxShader = skyboxMaterial.getShader();
    skyboxModel->addMaterial(skyboxMaterial);
}

Properties SkyboxNode::buildProperties() {
    Properties properties;

    properties.addSection("Skybox");
    properties.addProperty(std::make_unique<TextureProperty>(
        "Texture",
        [this] () { return skyboxTexture; },
        [this] (std::shared_ptr<Texture> tex)
        {
            skyboxTexture = tex;
            skyboxModel->getMaterial(0).setTexture("skybox", skyboxTexture);
        }
    ));

    return properties;
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

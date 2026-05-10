//
// Created by ruut on 5/10/26.
//

#ifndef SPHLASH_TEXTURE3DPANEL_H
#define SPHLASH_TEXTURE3DPANEL_H

#include <memory>

#include "UIPanel.h"
#include "glad/gl.h"
#include "rendering/Framebuffer.h"
#include "rendering/Shader.h"

class Texture3DPanel : public UIPanel {
public:
    explicit Texture3DPanel(const std::string& name, GLuint texture);

    void update(double deltaTime) override;
    void draw() override;

private:
    GLuint VAO;
    GLuint texture;
    Framebuffer* framebuffer = nullptr;
    std::shared_ptr<Shader> shader;

    float sliceZ = 0.0f;
};

#endif //SPHLASH_TEXTURE3DPANEL_H

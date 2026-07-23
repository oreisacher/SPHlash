//
// Created by ruut on 5/10/26.
//

#ifndef SPHLASH_TEXTURE3DPANEL_H
#define SPHLASH_TEXTURE3DPANEL_H

#include <memory>

#include "UIPanel.h"
#include "glad/gl.h"
#include "../../rendering/Framebuffer.h"
#include "../../assets/Shader.h"

class Texture3DPanel : public UIPanel {
public:
    explicit Texture3DPanel(const std::string& name, GLuint texture, int width, int height, int depth);

    void preDraw(double deltaTime) override;
    void draw() override;

private:
    GLuint VAO;
    GLuint texture;
    Framebuffer* framebuffer = nullptr;
    std::shared_ptr<Shader> shader;

    bool keepOpen = true;

    float totalDepth;
    float depth = 0;

    bool isDirty = true;
};

#endif //SPHLASH_TEXTURE3DPANEL_H

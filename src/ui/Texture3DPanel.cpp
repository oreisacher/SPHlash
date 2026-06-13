//
// Created by ruut on 5/10/26.
//

#include "Texture3DPanel.h"
#include "imgui.h"
#include "UIManager.h"

Texture3DPanel::Texture3DPanel(const std::string& name, GLuint texture, int width, int height, int depth)
    : UIPanel(name), texture(texture), totalDepth(depth)
{
    glGenVertexArrays(1, &VAO);

    framebuffer = new Framebuffer(width, height);
    framebuffer->addColorAttachment(GL_NEAREST);

    shader = Shader::createTexture3DSlice();
}

void Texture3DPanel::preDraw(double deltaTime) {
    if (!isDirty)
        return;

    // Render current slice into texture
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT);

    shader->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, texture);

    shader->setInt("tex", 0);
    shader->setFloat("sliceZ", depth / totalDepth);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    framebuffer->unbind();

    isDirty = false;
}

void Texture3DPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str(), &keepOpen);
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.y -= 30;

    ImGui::Image(
        (void *)(intptr_t)framebuffer->getColorAttachment(0),
        size,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    if (ImGui::SliderFloat("Depth", &depth, 0.0f, totalDepth)) {
        isDirty = true;
    }

    ImGui::End();

    // Close window on 'X' button click
    if (!keepOpen) {
        close();
    }
}

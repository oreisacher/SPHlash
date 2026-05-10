//
// Created by ruut on 5/10/26.
//

#include "Texture3DPanel.h"

#include "imgui.h"

Texture3DPanel::Texture3DPanel(const std::string& name, GLuint texture) : UIPanel(name), texture(texture) {
    glGenVertexArrays(1, &VAO);

    framebuffer = new Framebuffer(32, 32);
    framebuffer->addColorAttachment();

    shader = Shader::createTexture3DSlice();
}

void Texture3DPanel::update(double deltaTime) { }

void Texture3DPanel::draw() {
    if (!visible)
        return;

    // Render current slice into texture
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT);

    shader->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, texture);

    shader->setInt("tex", 0);
    shader->setFloat("sliceZ", sliceZ);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    framebuffer->unbind();

    ImGui::Begin(name.c_str());
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.y -= 30;

    ImGui::Image(
        (void *)(intptr_t)framebuffer->getColorAttachment(0),
        size,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    ImGui::SliderFloat("Z Slice", &sliceZ, 0.0f, 1.0f);

    ImGui::End();
}

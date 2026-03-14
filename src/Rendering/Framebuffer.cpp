//
// Created by ruut on 3/14/26.
//

#include "Framebuffer.h"
#include "glad/gl.h"

Framebuffer::Framebuffer(int width, int height) : width(width), height(height) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &fbo);

    for (auto tex : colorAttachments)
        glDeleteTextures(1, &tex);

    if (rbo)
        glDeleteRenderbuffers(1, &rbo);
}

GLuint Framebuffer::getID() const { return fbo; }
void Framebuffer::bind() const { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
void Framebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

GLuint Framebuffer::addColorAttachment() {
    // Create new texture add attach
    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0 + colorAttachments.size(),
        GL_TEXTURE_2D,
        texture,
        0
    );

    colorAttachments.push_back(texture);
    return texture;
}

GLuint Framebuffer::getColorAttachment(int index) const {
    if (index >= 0 && index < colorAttachments.size())
        return colorAttachments[index];
    return -1;
}

void Framebuffer::addDepthBuffer() {
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        rbo);
}

void Framebuffer::resize(int width, int height) {
    this->width = width;
    this->height = height;

    bind();

    // Resize color attachments
    for (GLuint tex : colorAttachments) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            width,
            height,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            nullptr
        );
    }

    // Resize depth buffer if existing
    if (rbo) {
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            width,
            height
        );
    }

    unbind();
}

glm::vec2 Framebuffer::getSize() {
    return { width, height };
}

bool Framebuffer::checkStatus() {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;
    return true;
}

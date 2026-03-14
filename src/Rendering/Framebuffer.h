//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_FRAMEBUFFER_H
#define FLUIDSIMSPH_FRAMEBUFFER_H

#include <vector>
#include "../../external/motor/external/glfw/src/internal.h"
#include "glm/vec2.hpp"

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    GLuint getID() const;

    void bind() const;
    static void unbind();

    GLuint addColorAttachment();
    GLuint getColorAttachment(int index) const;

    void addDepthBuffer();

    void resize(int width, int height);
    glm::vec2 getSize();

    bool checkStatus();

private:
    GLuint fbo = 0;
    GLuint rbo = 0;

    std::pmr::vector<GLuint> colorAttachments;

    int width = -1, height = -1;
};

#endif //FLUIDSIMSPH_FRAMEBUFFER_H
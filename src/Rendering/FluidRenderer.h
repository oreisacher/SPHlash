//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include "Framebuffer.h"
#include "../FluidSimulation.h"
#include "glad/gl.h"

class FluidRenderer {
public:
    FluidRenderer(const FluidSimulation* simulation);
    ~FluidRenderer();

    void render() const;

    GLuint getTextureId() const;
    void setSize(int width, int height) const;

private:
    const FluidSimulation* simulation;

    Framebuffer* framebuffer;
    GLuint vao, vbo;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
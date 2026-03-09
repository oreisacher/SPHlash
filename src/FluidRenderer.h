//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include "FluidSimulation.h"
#include "glad/gl.h"

class FluidRenderer {
public:
    FluidRenderer(const FluidSimulation* simulation);

    void update();
    void render();

private:
    const FluidSimulation* simulation;
    GLuint vao, vbo;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
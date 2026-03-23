//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include "Framebuffer.h"
#include "glad/gl.h"
#include "simulation/FluidSimulationGPU.h"

class FluidRenderer {
public:
    FluidRenderer(const FluidSimulationGPU* simulation);
    ~FluidRenderer();

    static void Init();

    void render() const;

    GLuint getTextureId() const;
    void setSize(int width, int height) const;

    GLuint getParticleVBO() const;

private:
    const FluidSimulationGPU* simulation;

    static inline bool initialized = false;

    Framebuffer* framebuffer;
    GLuint vao, vbo;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
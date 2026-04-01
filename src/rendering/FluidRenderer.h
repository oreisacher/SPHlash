//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include "Framebuffer.h"
#include "glad/gl.h"
#include "simulation/FluidSimulationGPU.h"
#include "utils/Camera.h"

struct RenderingSettings {
    float particleSize  = 0.005f;
};

class FluidRenderer {
public:
    FluidRenderer(FluidSimulationGPU* simulation);
    ~FluidRenderer();

    void render() const;

    void setCamera(Camera* cam);

    Shader* getParticleShader() const;
    Shader* getBoundaryShader() const;

    void updateParticleSize() const;
    void rebuildBoundaryBox() const;

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

    GLuint getParticleVBO() const;

    RenderingSettings settings;

private:
    void setupShaders();
    void setupSphereModel();
    void setupParticles();
    void setupBoundaryModel();

    FluidSimulationGPU* simulation;

    Camera* camera;

    Shader* particleShader;
    Shader* boundaryShader;

    Framebuffer* framebuffer;
    GLuint vao;

    // Sphere
    GLuint sphereVBO, sphereEBO;
    int sphereIndexCount;

    // Particle
    GLuint particleVBO;

    // Boundary
    GLuint boxVAO, boxVBO, boxEBO;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include <vector_functions.h>
#include <vector_types.h>
#include "Framebuffer.h"
#include "Mesh.h"
#include "Model.h"
#include "Shader.h"
#include "glad/gl.h"
#include "utils/Camera.h"

class Scene;
class FluidSimulation;

enum RenderMode {
    SOLID_COLOR_PARTICLES,
    VELOCITY_PARTICLES,
    PRESSURE_PARTICLES,
    SURFACE_PARTICLES,
    MARCHING_CUBES
};

struct RenderingSettings {
    float particleSize  = 0.005f;

    RenderMode mode = SOLID_COLOR_PARTICLES;

    // SOLID_COLOR_PARTICLES
    glm::vec4 solidColorParticles = glm::vec4(1.0, 0.0, 0.0, 1.0);

    // VELOCITY_PARTICLES
    glm::vec4 minVelColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    glm::vec4 maxVelColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    float velocityMin = 0.0f;
    float velocityMax = 1.0f;

    // PRESSURE_PARTICLES
    glm::vec4 minPresColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    glm::vec4 maxPresColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    float pressureMin = 0.0f;
    float pressureMax = 1.0f;

    // SURFACE_PARTICLES
    float maxSurfaceTension = 100.0f;
};

struct RendererParticle {
    float4 position;
    float4 velocity;
    float4 properties;
    float4 flags;
};

class FluidRenderer {
public:
    FluidRenderer(Scene& scene, FluidSimulation* simulation);
    ~FluidRenderer();

    void render(const Scene& scene) const;

    Shader* getParticleShader() const;

    void updateParticleSize() const;

    void updateParticleData() const;
    void updateRenderingSettings() const;

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

    GLuint getParticleVBO() const;

    RenderingSettings settings;

private:
    void setupParticles(Scene& scene);

    FluidSimulation* simulation;

    Model* particleModel                = nullptr;
    Shader* particleShader;

    Framebuffer* framebuffer;

    // Particle
    GLuint particleVBO;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
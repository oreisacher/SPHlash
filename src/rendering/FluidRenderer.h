//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include <vector_types.h>
#include "Framebuffer.h"
#include "Mesh.h"
#include "Model.h"
#include "Shader.h"
#include "glad/gl.h"
#include "utils/Camera.h"

class FluidSimulationGPU;

enum RenderMode {
    SOLID_COLOR_PARTICLES,
    VELOCITY_PARTICLES,
    PRESSURE_PARTICLES
};

struct RenderingSettings {
    float particleSize  = 0.005f;

    RenderMode mode = SOLID_COLOR_PARTICLES;

    // SOLID_COLOR_PARTICLES
    glm::vec3 solidColor = glm::vec3(1.0, 0.0, 0.0);

    // VELOCITY_PARTICLES
    glm::vec3 minVelColor = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 maxVelColor = glm::vec3(1.0f, 0.0f, 0.0f);
    float velocityMin = 0.0f;
    float velocityMax = 1.0f;

    // VELOCITY_PARTICLES
    glm::vec3 minPresColor = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 maxPresColor = glm::vec3(1.0f, 0.0f, 0.0f);
    float pressureMin = 0.0f;
    float pressureMax = 1.0f;
};

struct RendererParticle {
    float4 position;
    float4 velocity;
    float4 properties;
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
    void rebuildSpawnAreaBox() const;

    void updateParticleData() const;
    void updateRenderingSettings() const;

    void addModel(const std::string& path);
    void removeModel(int index);
    std::vector<Model*> getModels() const;

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

    GLuint getParticleVBO() const;

    RenderingSettings settings;

private:
    void setupShaders();
    void setupSphereModel();
    void setupParticles();
    void setupSpawnAreaModel();

    FluidSimulationGPU* simulation;

    Camera* camera;

    Shader* particleShader;
    Shader* boundaryShader;
    Shader* modelShader;

    Framebuffer* framebuffer;
    GLuint vao;

    // Sphere
    GLuint sphereVBO, sphereEBO;
    int sphereIndexCount;

    // Particle
    GLuint particleVBO;

    // Boundary
    GLuint boxVAO, boxVBO, boxEBO;

    // --- Models ---
    std::vector<Model*> models;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
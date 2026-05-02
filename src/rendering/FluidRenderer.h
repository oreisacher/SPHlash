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
#include "Scene.h"
#include "Shader.h"
#include "glad/gl.h"
#include "utils/Camera.h"

class FluidSimulation;

enum RenderMode {
    SOLID_COLOR_PARTICLES,
    VELOCITY_PARTICLES,
    PRESSURE_PARTICLES,
    SOLID_COLOR
};

struct RenderingSettings {
    float particleSize  = 0.005f;

    RenderMode mode = SOLID_COLOR_PARTICLES;

    // SOLID_COLOR_PARTICLES
    glm::vec3 solidColorParticles = glm::vec3(1.0, 0.0, 0.0);

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

    // SOLID_COLOR
    glm::vec3 solidColor = glm::vec3(1.0, 0.0, 0.0);
    int3 marchingCubeResolution = make_int3(128, 128, 128);
};

struct RendererParticle {
    float4 position;
    float4 velocity;
    float4 properties;
};

class FluidRenderer {
public:
    ~FluidRenderer();
    static FluidRenderer* getInstance();

    // void update();

    void render(const Scene& scene) const;

    Shader* getParticleShader() const;
    Shader* getBoundaryShader() const;

    void updateParticleSize() const;

    void updateParticleData() const;
    void updateRenderingSettings() const;

    void addModel(const std::string& path);
    void addModel(Model* model);
    void removeModel(int index);
    std::vector<Model*> getModels() const;

    void setSurfaceModel(Model* model);

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

    GLuint getParticleVBO() const;

    RenderingSettings settings;

private:
    FluidRenderer();

    void setupShaders();
    void setupSphereModel();
    void setupParticles();
    void setupSpawnAreaModel();

    glm::mat4 calculateModelMatrix(glm::vec3 min, glm::vec3 max);

    FluidSimulation* simulation;

    Shader* particleShader;
    Shader* boxShader;
    Shader* modelShader;

    Framebuffer* framebuffer;
    GLuint vao;

    // Sphere
    GLuint sphereVBO, sphereEBO;
    int sphereIndexCount;

    // Particle
    GLuint particleVBO;

    // Box Model
    GLuint boxVAO, boxVBO, boxEBO;

    // --- Models ---
    std::vector<Model*> editorModels;
    std::vector<Model*> models;

    Model* spawnAreaModel = nullptr;
    Model* surfaceModel = nullptr;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
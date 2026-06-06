//
// Created by ruut on 6/4/26.
//

#ifndef SPHLASH_PARTICLENODE_H
#define SPHLASH_PARTICLENODE_H

#include <vector_types.h>

#include "FluidSurfaceMesher.h"
#include "Framebuffer.h"
#include "glm/vec4.hpp"
#include "ui/SceneNode.h"

class Model;
class Shader;
class FluidSimulation;

enum RenderMode {
    SOLID_COLOR_PARTICLES,
    VELOCITY_PARTICLES,
    PRESSURE_PARTICLES,
    SURFACE_PARTICLES,
    MARCHING_CUBES,
    SCREEN_SPACE
};

struct RenderingSettings {
    float particleSize  = 0.005f;

    RenderMode mode = SOLID_COLOR_PARTICLES;

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

    // SCREEN_SPACE
    float billboardSize = 0.01f;
};

struct RendererParticle {
    float4 position;
    float4 velocity;
    float4 properties;
    float4 flags;
    float4 color;
};

class ParticlesNode : public SceneNode {
public:
    ParticlesNode(FluidSimulation* simulation);

    void draw(const glm::mat4& view, const glm::mat4& projection) const override;
    Properties buildProperties() override;

    Shader* getParticleShader() const;
    unsigned int getParticleVBO() const;

    void updateRenderingSettings() const;
    void updateParticlesSize(int count) const;

    FluidSurfaceMesher* getSurfaceMesher();
    int getRenderPriority() const override;

    RenderingSettings settings;

private:
    void setupInstancing();
    void updateSize(int width, int height) override;

    FluidSimulation* simulation;
    FluidSurfaceMesher mesher;

    std::shared_ptr<Model> particleModel;
    std::shared_ptr<Model> billboardParticleModel;
    Model* surfaceModel;

    Shader* particleShader;
    Shader* billboardParticleShader;

    unsigned int emptyVAO;

    std::unique_ptr<Framebuffer> sceneDepthBuffer;
    std::unique_ptr<Framebuffer> depthBuffer;
    std::unique_ptr<Framebuffer> smoothBuffer1;
    std::unique_ptr<Framebuffer> smoothBuffer2;
    std::unique_ptr<Framebuffer> normalBuffer;
    std::unique_ptr<Framebuffer> compositionBuffer;

    std::shared_ptr<Shader> depthShader;
    std::shared_ptr<Shader> blurShader;
    std::shared_ptr<Shader> normalShader;
    std::shared_ptr<Shader> compositionShader;
    std::shared_ptr<Shader> fullscreenShader;

    unsigned int particleVBO;
};

#endif //SPHLASH_PARTICLENODE_H

//
// Created by ruut on 6/7/26.
//

#ifndef SPHLASH_FLUIDSURFACESCREENSPACE_H
#define SPHLASH_FLUIDSURFACESCREENSPACE_H

#include <memory>
#include "Model.h"
#include "Shader.h"
#include "glm/glm.hpp"

class Framebuffer;

struct ScreenSpaceSettings {
    float particleSize      = 0.015;

    // Blur
    int blurSize            = 7;
    float depthThreshold    = 0.05;

    // Rendering
    glm::vec3 fluidColor    = { 0.0, 0.0, 1.0 };
    float transparency      = 1.0f;
    float ambientStrength   = 0.8f;
    float specularStrength  = 1.0f;
    float fresnelStrength   = 0.1f;
};

class FluidSurfaceScreenSpace {
public:
    FluidSurfaceScreenSpace();

    void draw(Framebuffer* dest, const glm::mat4& view, const glm::mat4& projection) const;

    void resizeBuffers(int width, int height) const;
    void updateParticleSize(int count) const;

    Mesh* getParticleMesh() const;

    void updateRenderingSettings() const;

    ScreenSpaceSettings settings;

private:
    void drawFullscreen() const;

    unsigned int screenVAO;

    std::shared_ptr<Model> quadParticleModel;
    Shader* particleShader;

    std::unique_ptr<Framebuffer> particlesDepthBuffer;
    std::unique_ptr<Framebuffer> blurBuffer1;
    std::unique_ptr<Framebuffer> blurBuffer2;
    std::unique_ptr<Framebuffer> normalBuffer;
    std::unique_ptr<Framebuffer> compositionBuffer;

    std::shared_ptr<Shader> blurShader;
    std::shared_ptr<Shader> normalShader;
    std::shared_ptr<Shader> compositionShader;
    std::shared_ptr<Shader> fullscreenShader;
};

#endif //SPHLASH_FLUIDSURFACESCREENSPACE_H

//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include "Framebuffer.h"
#include "simulation/scene/nodes/ParticlesNode.h"
#include "glad/gl.h"
#include "glm/fwd.hpp"

class Scene;
class FluidSimulation;

class Renderer {
public:
    Renderer(Scene* scene);
    ~Renderer();

    void render(const Scene* scene) const;

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

private:
    void setupParticles(Scene& scene, FluidSimulation* simulation);
    void renderNode(const SceneNode* node, const glm::mat4& view, const glm::mat4& projection) const;

    Framebuffer* framebuffer;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
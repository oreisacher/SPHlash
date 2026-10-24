//
// Created by ruut on 3/9/26.
//

#ifndef FLUIDSIMSPH_FLUIDRENDERER_H
#define FLUIDSIMSPH_FLUIDRENDERER_H

#include "Framebuffer.h"
#include "../simulation/scene/nodes/ParticlesNode.h"
#include "glad/gl.h"
#include "glm/fwd.hpp"

class Scene;
class SimulationNode;

struct RendererSettings {
    bool autoResizeViewport = true;

    float viewportScaling = 1.0f;
    int viewportWidth = 1280;
    int viewportHeight = 720;
};

class Renderer {
public:
    Renderer(Scene* scene);
    ~Renderer();

    void render(const Scene* scene) const;

    // Framebuffer texture
    GLuint getTextureId() const;
    Framebuffer* getFramebuffer() const;

    void setSize(int width, int height) const;
    void updateSize() const;

    RendererSettings settings;

private:
    void renderNode(IRenderable* renderable, const glm::mat4& view, const glm::mat4& projection);

    Framebuffer* framebuffer;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
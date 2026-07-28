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

class Renderer {
public:
    Renderer(Scene* scene);
    ~Renderer();

    void render(const Scene* scene) const;

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

private:
    void renderNode(IRenderable* renderable, const glm::mat4& view, const glm::mat4& projection);

    Framebuffer* framebuffer;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
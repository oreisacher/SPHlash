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
#include "ModelNode.h"
#include "ParticlesNode.h"
#include "Shader.h"
#include "glad/gl.h"
#include "utils/Camera.h"

class Scene;
class FluidSimulation;

class Renderer {
public:
    Renderer(Scene* scene);
    ~Renderer();

    void render(const Scene& scene) const;

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

private:
    void setupParticles(Scene& scene, FluidSimulation* simulation);
    void renderNode(const SceneNode* node, const glm::mat4& view, const glm::mat4& projection) const;

    Framebuffer* framebuffer;
};

#endif //FLUIDSIMSPH_FLUIDRENDERER_H
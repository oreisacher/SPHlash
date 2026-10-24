//
// Created by ruut on 3/9/26.
//

#define GLAD_GL_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL

#include "rendering/Renderer.h"
#include <glad/gl.h>
#include <vector>

#include "rendering/IRenderable.h"
#include "simulation/scene/Scene.h"
#include "simulation/scene/nodes/SkyboxNode.h"

Renderer::Renderer(Scene* scene) {
    // Setup OpenGL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup framebuffer
    framebuffer = new Framebuffer(1, 1);
    framebuffer->bind();
    framebuffer->addColorAttachment(GL_LINEAR);
    framebuffer->addDepthTexture();
}

Renderer::~Renderer() {
    delete framebuffer;
}

void Renderer::renderNode(IRenderable* renderable, const glm::mat4& view, const glm::mat4& projection) {
    // auto node = renderable->getNode();
    //
    // if (!node->isVisible())
    //     return;
    //
    // renderable->draw(view, projection);
}

void Renderer::render(const Scene* scene) const {
    // Render into framebuffer
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera transform
    auto camera = scene->getCamera();
    auto view = camera->getViewMatrix();
    auto projection = camera->getProjectionMatrix();

    std::vector<IRenderable*> renderables;
    for (SceneNode* node : scene->getSceneNodes())
        if (auto* r = dynamic_cast<IRenderable*>(node))
            renderables.push_back(r);

    std::ranges::sort(renderables, [](auto* a, auto* b) {
        return a->getRenderPriority() < b->getRenderPriority();
    });

    for (auto* r : renderables) {
        auto node = r->getNode();

        if (!node->isVisible())
            continue;

        r->draw(view, projection);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Renderer::getTextureId() const {
    return framebuffer->getColorAttachment(0);
}

Framebuffer* Renderer::getFramebuffer() const {
    return framebuffer;
}

void Renderer::setSize(int width, int height) const {
    auto size = framebuffer->getSize();
    if (size.x != width || size.y != height)
        framebuffer->resize(width, height);
}

void Renderer::updateSize() const {
    framebuffer->resize(
        settings.viewportScaling * settings.viewportWidth,
        settings.viewportScaling * settings.viewportHeight
    );
}

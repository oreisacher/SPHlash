//
// Created by ruut on 6/4/26.
//

#ifndef SPHLASH_PARTICLENODE_H
#define SPHLASH_PARTICLENODE_H

#include <vector_types.h>
#include "../../../rendering/FluidSurfaceMesher.h"
#include "rendering/FluidSurfaceScreenSpace.h"
#include "glm/vec4.hpp"
#include "SceneNode.h"

class Model;
class Shader;
class FluidSimulation;

enum RenderMode {
    SOLID_COLOR_PARTICLES,
    VELOCITY_PARTICLES,
    PRESSURE_PARTICLES,
    SURFACE_PARTICLES,
    TOP_SURFACE_PARTICLES,
    MARCHING_CUBES,
    SCREEN_SPACE
};

struct RenderingSettings {
    float particleSize  = 0.005f;

    RenderMode mode = SOLID_COLOR_PARTICLES;

    // VELOCITY_PARTICLES
    glm::vec3 minVelColor = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 maxVelColor = glm::vec3(1.0f, 0.0f, 0.0f);
    float velocityMin = 0.0f;
    float velocityMax = 1.0f;

    // PRESSURE_PARTICLES
    glm::vec3 minPresColor = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 maxPresColor = glm::vec3(1.0f, 0.0f, 0.0f);
    float pressureMin = 0.0f;
    float pressureMax = 1.0f;

    // SURFACE_PARTICLES
    float maxSurfaceTension = 2000.0f;
};

/*
    This node is responsible for drawing the simulations particles.
    Simple particle rendering is done within this node while more complex rendering e.g.
    generating the surface via Marching Cubes algorithm or Screen Space method is delegated to
    the respective class.
*/
class ParticlesNode : public SceneNode {
public:
    ParticlesNode(FluidSimulation* simulation);

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;
    Properties buildProperties() override;

    Shader* getParticleShader() const;
    unsigned int getParticleVBO() const;

    void updateRenderingSettings() const;
    void updateParticlesSize(int count) const;

    FluidSurfaceMesher* getSurfaceMesher();
    FluidSurfaceScreenSpace* getSurfaceScreenSpace();

    int getRenderPriority() const override;
    void updateSize(int width, int height) override;

    RenderingSettings settings;

private:
    void setupInstancing();

    FluidSimulation* simulation;

    FluidSurfaceMesher mesher;
    FluidSurfaceScreenSpace screenSpace;

    std::shared_ptr<Model> particleModel;

    Model* surfaceModel;

    Shader* particleShader;

    unsigned int particleVBO;
};

#endif //SPHLASH_PARTICLENODE_H

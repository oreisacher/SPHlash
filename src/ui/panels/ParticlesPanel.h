//
// Created by ruut on 3/24/26.
//

#ifndef FLUIDSIMSPH_PARTICLESPANEL_H
#define FLUIDSIMSPH_PARTICLESPANEL_H

#include "UIPanel.h"
#include "simulation/FluidSimulation.h"

class Shader;
class Renderer;
class FluidSimulation;

class ParticlesPanel : public UIPanel {
public:
    explicit ParticlesPanel(FluidSimulation* simulation, ParticlesNode* particles);

    void preDraw(double deltaTime) override;
    void draw() override;

private:
    FluidSimulation* simulation;
    Shader* particlesShader;

    ParticleHost particle;

    double time = 0.0f;

    bool showParticles = false;
    int selectedParticle = 0;
};

#endif //FLUIDSIMSPH_PARTICLESPANEL_H
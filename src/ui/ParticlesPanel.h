//
// Created by ruut on 3/24/26.
//

#ifndef FLUIDSIMSPH_PARTICLESPANEL_H
#define FLUIDSIMSPH_PARTICLESPANEL_H

#include "UIPanel.h"
#include "glad/gl.h"
#include "rendering/FluidRenderer.h"
#include "rendering/Shader.h"
#include "simulation/FluidSimulationGPU.h"

class ParticlesPanel : public UIPanel {
public:
    explicit ParticlesPanel();

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSimulationGPU* simulation;
    FluidRenderer* renderer;

    ParticleGPU particle;

    double time = 0.0f;

    bool showParticles = false;
    int selectedParticle = 0;
};

#endif //FLUIDSIMSPH_PARTICLESPANEL_H
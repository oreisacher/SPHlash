//
// Created by ruut on 3/24/26.
//

#ifndef FLUIDSIMSPH_PARTICLESPANEL_H
#define FLUIDSIMSPH_PARTICLESPANEL_H

#include <cmath>
#include "UIPanel.h"
#include "core/Editor.h"
#include "simulation/SimulationNode.h"

class Shader;
class Renderer;
class SimulationNode;

class ParticlesPanel : public UIPanel {
public:
    explicit ParticlesPanel(const Editor* editor);

    void update(double deltaTime) override;
    void draw() override;

private:
    SimulationNode* simulation = nullptr;
    Shader* particlesShader;

    ParticleHost particle;

    double time = 0.0f;

    bool showParticles = false;
    int selectedParticle = 0;
};

#endif //FLUIDSIMSPH_PARTICLESPANEL_H
//
// Created by ruut on 4/10/26.
//

#ifndef FLUIDSIMSPH_INITPANEL_H
#define FLUIDSIMSPH_INITPANEL_H

#include "UIPanel.h"
#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulationGPU.h"

class InitPanel : public UIPanel{
public:
    explicit InitPanel();

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSimulationGPU* simulation;
    FluidRenderer* renderer;

    int particlesToSpawn = 0;
    float spawnVolume = 0;
    float massPerParticle = 0;

    void recalculateParticles();
};

#endif //FLUIDSIMSPH_INITPANEL_H

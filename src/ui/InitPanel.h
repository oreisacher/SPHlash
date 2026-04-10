//
// Created by ruut on 4/10/26.
//

#ifndef FLUIDSIMSPH_INITPANEL_H
#define FLUIDSIMSPH_INITPANEL_H

#include "UIPanel.h"
#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulationGPU.h"

//
// Boundary of spawn area
// Mass of spawning area
// => Call init particles function again on button press
//
class InitPanel : public UIPanel{
public:
    explicit InitPanel(const std::string& name, FluidSimulationGPU* simulation, FluidRenderer* renderer);

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

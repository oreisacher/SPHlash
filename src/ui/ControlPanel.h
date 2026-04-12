//
// Created by ruut on 4/10/26.
//

#ifndef FLUIDSIMSPH_CONTROLPANEL_H
#define FLUIDSIMSPH_CONTROLPANEL_H

#include "UIPanel.h"
#include "simulation/FluidSimulationGPU.h"

class ControlPanel : public UIPanel {
public:
    explicit ControlPanel(const std::string& name, FluidSimulationGPU* simulation);

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSimulationGPU* simulation;

    float timeStep = 0.0;
    float stepSize = 16.666667;
};

#endif //FLUIDSIMSPH_CONTROLPANEL_H

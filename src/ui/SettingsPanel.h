//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_SETTINGSPANEL_H
#define FLUIDSIMSPH_SETTINGSPANEL_H

#include "UIPanel.h"
#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulation.h"
#include "simulation/FluidSimulationGPU.h"

class SettingsPanel : public UIPanel {
public:
    explicit SettingsPanel(const std::string& name, FluidSimulationGPU* simulation, FluidRenderer* renderer);

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidRenderer* renderer;
    FluidSimulationGPU* simulation;
    SimulationSettings* settings;

    void SaveSettings(const std::string& path) const;
    void LoadSettings(const std::string& path);
};

#endif //FLUIDSIMSPH_SETTINGSPANEL_H
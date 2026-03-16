//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_SETTINGSPANEL_H
#define FLUIDSIMSPH_SETTINGSPANEL_H

#include "UIPanel.h"
#include "simulation/FluidSimulation.h"

class SettingsPanel : public UIPanel {
public:
    explicit SettingsPanel(const std::string& name, FluidSimulation* simulation);

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSimulation* simulation;
    SimulationSettings* settings;

    void SaveSettings(const std::string& path) const;
    void LoadSettings(const std::string& path) const;
};

#endif //FLUIDSIMSPH_SETTINGSPANEL_H
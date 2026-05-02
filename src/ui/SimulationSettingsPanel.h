//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_SETTINGSPANEL_H
#define FLUIDSIMSPH_SETTINGSPANEL_H

#include "UIPanel.h"
#include "rendering/Editor.h"
#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulation.h"
#include "simulation/FluidSimulation.h"

class SimulationSettingsPanel : public UIPanel {
public:
    explicit SimulationSettingsPanel(Editor* editor);

    void update(double deltaTime) override;
    void draw() override;

private:
    Editor* editor;
    FluidSimulation* simulation;
    FluidRenderer* renderer;

    void SaveSettings(const std::string& path) const;
    void LoadSettings(const std::string& path);
};

#endif //FLUIDSIMSPH_SETTINGSPANEL_H